#include "parse.hpp"
#include <windows.h>
#include <winhttp.h>
#include <shlobj.h>
#include <fstream>
#include <filesystem>
#include <charconv>
#include <unordered_map>
#include <thread>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <span>
#include "../game/entity_cache.hpp"
#include "../game/sdk.hpp"

#pragma comment(lib, "winhttp.lib")

namespace mesh_parser {

    constexpr size_t THREAD_POOL_SIZE = 6;
    constexpr uint32_t BINARY_CACHE_MAGIC = 0x4D455348; // mesh
    constexpr uint32_t BINARY_CACHE_VERSION = 2;

    namespace internal {
        std::unordered_map<uint64_t, std::shared_ptr<mesh_t>> g_mesh_cache;
        std::shared_mutex g_mesh_cache_mutex;

        std::queue<uint64_t> g_task_queue;
        std::unordered_map<uint64_t, bool> g_pending_assets;
        std::mutex g_queue_mutex;
        std::condition_variable g_queue_cv;

        std::vector<std::jthread> g_worker_threads;
        std::atomic<bool> g_running{ false };

        // ufmem
        // [mesh_parser::read_le]
        template<typename T>
        inline T read_le(std::span<const uint8_t> data, size_t& offset) {
            T value;
            if (offset + sizeof(T) <= data.size()) {
                std::memcpy(&value, data.data() + offset, sizeof(T));
                offset += sizeof(T);
                return value;
            }
            return T();
        }

        // [mesh_parser::get_storage_directory]
        inline std::string get_storage_directory() {
            char appdata_path[MAX_PATH];
            if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appdata_path) == S_OK) {
                return std::string(appdata_path) + "\\violet\\mesh_cache";
            }
            return "";
        }

        // [mesh_parser::ensure_storage_directory]
        inline bool ensure_storage_directory() {
            std::string dir = get_storage_directory();
            if (dir.empty()) return false;
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            return !ec;
        }

        // [mesh_parser::compute_mesh_bounds]
        void compute_mesh_bounds(mesh_t& mesh) {
            if (mesh.vertices.empty()) {
                mesh.bounds.is_valid = false;
                return;
            }

            mesh.bounds.min = { FLT_MAX, FLT_MAX, FLT_MAX };
            mesh.bounds.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

            for (const auto& v : mesh.vertices) {
                mesh.bounds.min.x = (std::min)(mesh.bounds.min.x, v.position.x);
                mesh.bounds.min.y = (std::min)(mesh.bounds.min.y, v.position.y);
                mesh.bounds.min.z = (std::min)(mesh.bounds.min.z, v.position.z);
                
                mesh.bounds.max.x = (std::max)(mesh.bounds.max.x, v.position.x);
                mesh.bounds.max.y = (std::max)(mesh.bounds.max.y, v.position.y);
                mesh.bounds.max.z = (std::max)(mesh.bounds.max.z, v.position.z);
            }

            mesh.bounds.size = mesh.bounds.max - mesh.bounds.min;
            mesh.bounds.center = (mesh.bounds.min + mesh.bounds.max) * 0.5f;
            mesh.bounds.is_valid = true;
        }

        // [mesh_parser::parse_mesh_v1]
        bool parse_mesh_v1(std::span<const uint8_t> data, size_t offset, mesh_t& mesh, float scale, bool invert_uv) {
            std::string_view text(reinterpret_cast<const char*>(data.data() + offset), data.size() - offset);
            
            // vtex
            auto newline_idx = text.find('\n');
            if (newline_idx == std::string_view::npos) return false;
            
            uint32_t num_faces = 0;
            std::from_chars(text.data(), text.data() + newline_idx, num_faces);
            if (num_faces == 0) return false;

            auto next_float = [](std::string_view& view, float& val) -> bool {
                while (!view.empty() && (view[0] < '0' || view[0] > '9') && view[0] != '-' && view[0] != '.') view.remove_prefix(1);
                if (view.empty()) return false;
                auto [ptr, ec] = std::from_chars(view.data(), view.data() + view.size(), val);
                if (ec == std::errc()) {
                    view.remove_prefix(ptr - view.data());
                    return true;
                }
                return false;
            };

            mesh.vertices.reserve(num_faces * 3);
            mesh.faces.reserve(num_faces);
            
            std::string_view remains = text.substr(text.find('['));
            for (uint32_t i = 0; i < num_faces; i++) {
                face_t face;
                for (int v = 0; v < 3; v++) {
                    vertex_t vert{};
                    float temp[9]; // pos(3), norm(3), uv(2), tangent(1)
                    
                    for (int j = 0; j < 9; j++) {
                        if (!next_float(remains, temp[j])) return false;
                    }
                    
                    vert.position = { temp[0] * scale, temp[1] * scale, temp[2] * scale };
                    vert.normal = { temp[3], temp[4], temp[5] };
                    float uv_y = temp[7];
                    vert.uv = { temp[6], invert_uv ? (1.0f - uv_y) : uv_y };
                    vert.tangent = { 0, 0, 0 };
                    vert.r = vert.g = vert.b = vert.a = 255;
                    
                    mesh.vertices.push_back(vert);
                }
                face.a = i * 3;
                face.b = i * 3 + 1;
                face.c = i * 3 + 2;
                mesh.faces.push_back(face);
            }
            return true;
        }

        // [mesh_parser::parse_mesh_v2]
        bool parse_mesh_v2(std::span<const uint8_t> data, size_t offset, mesh_t& mesh) {
            size_t pos = offset;
            if (pos + 12 > data.size()) return false;

            uint16_t cb_size = read_le<uint16_t>(data, pos);
            if (cb_size != 12) return false;

            uint8_t cb_vertices_stride = read_le<uint8_t>(data, pos);
            pos += 1; // cb_face_stride
            uint32_t num_vertices = read_le<uint32_t>(data, pos);
            uint32_t num_faces = read_le<uint32_t>(data, pos);

            bool has_rgba = (cb_vertices_stride == 40);

            mesh.vertices.reserve(num_vertices);
            for (uint32_t i = 0; i < num_vertices; i++) {
                if (pos + cb_vertices_stride > data.size()) return false;
                vertex_t v{};
                v.position = { read_le<float>(data, pos), read_le<float>(data, pos), read_le<float>(data, pos) };
                v.normal = { read_le<float>(data, pos), read_le<float>(data, pos), read_le<float>(data, pos) };
                v.uv = { read_le<float>(data, pos), read_le<float>(data, pos) };
                
                v.tangent.x = read_le<int8_t>(data, pos) / 127.0f;
                v.tangent.y = read_le<int8_t>(data, pos) / 127.0f;
                v.tangent.z = read_le<int8_t>(data, pos) / 127.0f;
                pos += 1; // ts

                if (has_rgba) {
                    v.r = read_le<uint8_t>(data, pos);
                    v.g = read_le<uint8_t>(data, pos);
                    v.b = read_le<uint8_t>(data, pos);
                    v.a = read_le<uint8_t>(data, pos);
                } else {
                    v.r = v.g = v.b = v.a = 255;
                }
                mesh.vertices.push_back(v);
            }

            mesh.faces.reserve(num_faces);
            for (uint32_t i = 0; i < num_faces; i++) {
                if (pos + 12 > data.size()) return false;
                mesh.faces.push_back({ read_le<uint32_t>(data, pos), read_le<uint32_t>(data, pos), read_le<uint32_t>(data, pos) });
            }
            return true;
        }

        // [mesh_parser::parse_mesh_v3]
        bool parse_mesh_v3(std::span<const uint8_t> data, size_t offset, mesh_t& mesh) {
            size_t pos = offset;
            if (pos + 16 > data.size()) return false;

            uint16_t cb_size = read_le<uint16_t>(data, pos);
            if (cb_size != 16) return false;

            pos += 2; // strides
            pos += 2; // sizeof_lod
            uint16_t num_lods = read_le<uint16_t>(data, pos);
            uint32_t num_vertices = read_le<uint32_t>(data, pos);
            uint32_t num_faces = read_le<uint32_t>(data, pos);

            mesh.vertices.reserve(num_vertices);
            for (uint32_t i = 0; i < num_vertices; i++) {
                if (pos + 40 > data.size()) return false;
                vertex_t v{};
                v.position = { read_le<float>(data, pos), read_le<float>(data, pos), read_le<float>(data, pos) };
                v.normal = { read_le<float>(data, pos), read_le<float>(data, pos), read_le<float>(data, pos) };
                v.uv = { read_le<float>(data, pos), read_le<float>(data, pos) };
                v.tangent.x = read_le<int8_t>(data, pos) / 127.0f;
                v.tangent.y = read_le<int8_t>(data, pos) / 127.0f;
                v.tangent.z = read_le<int8_t>(data, pos) / 127.0f;
                pos += 1;
                v.r = read_le<uint8_t>(data, pos);
                v.g = read_le<uint8_t>(data, pos);
                v.b = read_le<uint8_t>(data, pos);
                v.a = read_le<uint8_t>(data, pos);
                mesh.vertices.push_back(v);
            }

            mesh.faces.reserve(num_faces);
            for (uint32_t i = 0; i < num_faces; i++) {
                if (pos + 12 > data.size()) return false;
                mesh.faces.push_back({ read_le<uint32_t>(data, pos), read_le<uint32_t>(data, pos), read_le<uint32_t>(data, pos) });
            }

            for (uint16_t i = 0; i < num_lods; i++) {
                if (pos + 4 <= data.size()) mesh.lods.push_back(read_le<uint32_t>(data, pos));
            }
            if (mesh.lods.size() > 1 && mesh.lods[1] < mesh.faces.size()) mesh.faces.resize(mesh.lods[1]);

            return true;
        }

        // [mesh_parser::parse_mesh_v4_v5]
        bool parse_mesh_v4_v5(std::span<const uint8_t> data, size_t offset, mesh_t& mesh, uint16_t header_size) {
            size_t pos = offset;
            if (pos + header_size > data.size()) return false;

            uint16_t cb_size = read_le<uint16_t>(data, pos);
            if (cb_size != header_size) return false;

            pos += 2; // lod_type
            uint32_t num_verts = read_le<uint32_t>(data, pos);
            uint32_t num_faces = read_le<uint32_t>(data, pos);
            uint16_t num_lods = read_le<uint16_t>(data, pos);
            uint16_t num_bones = read_le<uint16_t>(data, pos);
            
            if (header_size == 32) pos += 18;
            else pos += 10;

            mesh.vertices.reserve(num_verts);
            for (uint32_t i = 0; i < num_verts; i++) {
                if (pos + 40 > data.size()) return false;
                vertex_t v{};
                v.position = { read_le<float>(data, pos), read_le<float>(data, pos), read_le<float>(data, pos) };
                v.normal = { read_le<float>(data, pos), read_le<float>(data, pos), read_le<float>(data, pos) };
                v.uv = { read_le<float>(data, pos), read_le<float>(data, pos) };
                v.tangent.x = read_le<int8_t>(data, pos) / 127.0f;
                v.tangent.y = read_le<int8_t>(data, pos) / 127.0f;
                v.tangent.z = read_le<int8_t>(data, pos) / 127.0f;
                pos += 1;
                v.r = read_le<uint8_t>(data, pos);
                v.g = read_le<uint8_t>(data, pos);
                v.b = read_le<uint8_t>(data, pos);
                v.a = read_le<uint8_t>(data, pos);
                mesh.vertices.push_back(v);
            }

            if (num_bones > 0) pos += num_verts * 8; // skip envelops

            mesh.faces.reserve(num_faces);
            for (uint32_t i = 0; i < num_faces; i++) {
                if (pos + 12 > data.size()) return false;
                mesh.faces.push_back({ read_le<uint32_t>(data, pos), read_le<uint32_t>(data, pos), read_le<uint32_t>(data, pos) });
            }

            for (uint16_t i = 0; i < num_lods; i++) {
                if (pos + 4 <= data.size()) mesh.lods.push_back(read_le<uint32_t>(data, pos));
            }
            if (mesh.lods.size() > 1 && mesh.lods[1] < mesh.faces.size()) mesh.faces.resize(mesh.lods[1]);

            return true;
        }

        // [mesh_parser::parse_mesh_from_data]
        bool parse_mesh_from_data(std::span<const uint8_t> data, mesh_t& mesh) {
            if (data.size() < 12) return false;
            
            std::string header(reinterpret_cast<const char*>(data.data()), 12);
            if (!header.starts_with("version ")) return false;
            mesh.version = header.substr(8, 4);

            size_t offset = 0;
            for (size_t i = 0; i < data.size(); i++) {
                if (data[i] == '\n') {
                    offset = i + 1;
                    break;
                }
            }

            bool success = false;
            if (mesh.version == "1.00") success = parse_mesh_v1(data, offset, mesh, 0.5f, true);
            else if (mesh.version == "1.01") success = parse_mesh_v1(data, offset, mesh, 1.0f, false);
            else if (mesh.version == "2.00") success = parse_mesh_v2(data, offset, mesh);
            else if (mesh.version == "3.00" || mesh.version == "3.01") success = parse_mesh_v3(data, offset, mesh);
            else if (mesh.version == "4.00" || mesh.version == "4.01") success = parse_mesh_v4_v5(data, offset, mesh, 24);
            else if (mesh.version == "5.00" || mesh.version == "5.01" || mesh.version == "6.00" || mesh.version == "7.00") success = parse_mesh_v4_v5(data, offset, mesh, 32);

            if (success) compute_mesh_bounds(mesh);
            return success;
        }

        // [mesh_parser::download_from_url]
        bool download_from_url(const std::wstring& host, const std::wstring& path, std::vector<uint8_t>& out_data) {
            HINTERNET hSession = WinHttpOpen(L"Violet/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!hSession) return false;

            DWORD timeout = 10000;
            WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
            WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
            WinHttpSetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));

            HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
            if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
            if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

            WinHttpAddRequestHeaders(hRequest, L"Accept-Encoding: identity", -1, WINHTTP_ADDREQ_FLAG_REPLACE | WINHTTP_ADDREQ_FLAG_ADD);

            if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false;
            }

            if (!WinHttpReceiveResponse(hRequest, NULL)) {
                WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false;
            }

            DWORD status = 0;
            DWORD status_size = sizeof(status);
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &status_size, NULL) && status != 200) {
                WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false;
            }

            DWORD size = 0, downloaded = 0;
            std::vector<uint8_t> compressed;
            do {
                size = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &size) || size == 0) break;
                
                size_t old_size = compressed.size();
                compressed.resize(old_size + size);
                if (!WinHttpReadData(hRequest, compressed.data() + old_size, size, &downloaded)) {
                    compressed.resize(old_size);
                    break;
                }
                if (downloaded < size) compressed.resize(old_size + downloaded);
            } while (size > 0);

            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);

            if (compressed.empty()) return false;

            out_data = std::move(compressed);
            return true;
        }

        // [mesh_parser::save_cache]
        bool save_cache(uint64_t asset_id, const mesh_t& mesh) {
            ensure_storage_directory();
            std::string path = get_storage_directory() + "\\" + std::to_string(asset_id) + ".mesh";
            std::ofstream file(path, std::ios::binary);
            if (!file) return false;

            file.write(reinterpret_cast<const char*>(&BINARY_CACHE_MAGIC), 4);
            file.write(reinterpret_cast<const char*>(&BINARY_CACHE_VERSION), 4);
            
            uint32_t len = static_cast<uint32_t>(mesh.version.size());
            file.write(reinterpret_cast<const char*>(&len), 4);
            file.write(mesh.version.data(), len);

            uint32_t num_v = static_cast<uint32_t>(mesh.vertices.size());
            uint32_t num_f = static_cast<uint32_t>(mesh.faces.size());
            uint32_t num_l = static_cast<uint32_t>(mesh.lods.size());

            file.write(reinterpret_cast<const char*>(&num_v), 4);
            file.write(reinterpret_cast<const char*>(&num_f), 4);
            file.write(reinterpret_cast<const char*>(&num_l), 4);

            if (num_v) file.write(reinterpret_cast<const char*>(mesh.vertices.data()), num_v * sizeof(vertex_t));
            if (num_f) file.write(reinterpret_cast<const char*>(mesh.faces.data()), num_f * sizeof(face_t));
            if (num_l) file.write(reinterpret_cast<const char*>(mesh.lods.data()), num_l * sizeof(uint32_t));

            return true;
        }

        // [mesh_parser::load_cache]
        bool load_cache(uint64_t asset_id, mesh_t& mesh) {
            std::string path = get_storage_directory() + "\\" + std::to_string(asset_id) + ".mesh";
            std::ifstream file(path, std::ios::binary);
            if (!file) return false;

            uint32_t magic, version;
            file.read(reinterpret_cast<char*>(&magic), 4);
            file.read(reinterpret_cast<char*>(&version), 4);
            if (magic != BINARY_CACHE_MAGIC || version != BINARY_CACHE_VERSION) return false;

            uint32_t len;
            file.read(reinterpret_cast<char*>(&len), 4);
            mesh.version.resize(len);
            file.read(mesh.version.data(), len);

            uint32_t num_v, num_f, num_l;
            file.read(reinterpret_cast<char*>(&num_v), 4);
            file.read(reinterpret_cast<char*>(&num_f), 4);
            file.read(reinterpret_cast<char*>(&num_l), 4);

            if (num_v) { mesh.vertices.resize(num_v); file.read(reinterpret_cast<char*>(mesh.vertices.data()), num_v * sizeof(vertex_t)); }
            if (num_f) { mesh.faces.resize(num_f); file.read(reinterpret_cast<char*>(mesh.faces.data()), num_f * sizeof(face_t)); }
            if (num_l) { mesh.lods.resize(num_l); file.read(reinterpret_cast<char*>(mesh.lods.data()), num_l * sizeof(uint32_t)); }

            compute_mesh_bounds(mesh);
            return true;
        }

        // [mesh_parser::try_get_mesh_from_memory]
        bool try_get_mesh_from_memory(uint64_t asset_id, std::vector<uint8_t>& out_data) {
            auto snap = cache::get_snapshot();
            for (const auto& player : snap->players) {
                for (int b = 0; b < BONE_COUNT; ++b) {
                    if (player.bones[b].mesh.asset_id == asset_id) {
                        uintptr_t inst = player.bones[b].instance;
                        if (!inst) continue;

                        uintptr_t mesh_ptr = 0;
                        if (player.bones[b].mesh.is_mesh_part) {
                            mesh_ptr = mem.read<uintptr_t>(inst + 0x300);
                        } else if (player.bones[b].mesh.special_mesh_ptr) {
                            mesh_ptr = mem.read<uintptr_t>(player.bones[b].mesh.special_mesh_ptr + 0x138);
                        }

                        if (mesh_ptr) {
                            uintptr_t mesh_content = mem.read<uintptr_t>(mesh_ptr + 0x20);
                            if (!mesh_content) mesh_content = mem.read<uintptr_t>(mesh_ptr + 0x18);
                            
                            if (mesh_content) {
                                uintptr_t shared_string = mem.read<uintptr_t>(mesh_content + 0x10);
                                if (shared_string) {
                                    uintptr_t data_ptr = mem.read<uintptr_t>(shared_string + 0x10);
                                    size_t data_size = mem.read<size_t>(shared_string + 0x18);
                                    
                                    if (data_ptr && data_size > 0 && data_size < 50 * 1024 * 1024) {
                                        out_data.resize(data_size);
                                        SIZE_T read = 0;
                                        auto h = mem.process_handle;
                                        if (h && Hagi_ReadVirtualMemory(h.get(), (PVOID)data_ptr, out_data.data(), data_size, &read) == 0 && read == data_size) {
                                            if (out_data.size() >= 12 && std::string_view(reinterpret_cast<const char*>(out_data.data()), 8) == "version ") {
                                                return true;
                                            }
                                        }
                                        out_data.clear();
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return false;
        }

        // [mesh_parser::download_worker]
        void download_worker() {
            while (g_running) {
                uint64_t asset_id = 0;
                {
                    std::unique_lock<std::mutex> lock(g_queue_mutex);
                    g_queue_cv.wait(lock, [] { return !g_task_queue.empty() || !g_running; });
                    if (!g_running && g_task_queue.empty()) break;
                    
                    asset_id = g_task_queue.front();
                    g_task_queue.pop();
                }

                auto clear_pending = [&]() {
                    std::lock_guard<std::mutex> lock(g_queue_mutex);
                    g_pending_assets.erase(asset_id);
                };

                auto mesh = std::make_shared<mesh_t>();
                if (load_cache(asset_id, *mesh)) {
                    std::unique_lock<std::shared_mutex> lock(g_mesh_cache_mutex);
                    g_mesh_cache[asset_id] = mesh;
                    clear_pending();
                    continue;
                }

                std::vector<uint8_t> data;
                if (try_get_mesh_from_memory(asset_id, data)) {
                    if (parse_mesh_from_data(data, *mesh)) {
                        save_cache(asset_id, *mesh);
                        {
                            std::unique_lock<std::shared_mutex> lock(g_mesh_cache_mutex);
                            g_mesh_cache[asset_id] = mesh;
                        }
                        clear_pending();
                        continue;
                    }
                }

                std::wstring path = L"/v1/asset/?id=" + std::to_wstring(asset_id);
                if (!download_from_url(L"assetdelivery.roblox.com", path, data)) {
                    clear_pending();
                    continue;
                }

                if (!parse_mesh_from_data(data, *mesh)) {
                    clear_pending();
                    continue;
                }

                save_cache(asset_id, *mesh);
                {
                    std::unique_lock<std::shared_mutex> lock(g_mesh_cache_mutex);
                    g_mesh_cache[asset_id] = mesh;
                }
                clear_pending();
            }
        }
    }

    // [mesh_parser::initialize]
    void initialize() {
        if (internal::g_running) return;
        internal::g_running = true;
        for (size_t i = 0; i < THREAD_POOL_SIZE; i++) {
            internal::g_worker_threads.emplace_back(internal::download_worker);
        }
    }

    // [mesh_parser::get_mesh]
    std::shared_ptr<mesh_t> get_mesh(uint64_t asset_id) {
        if (asset_id == 0) return nullptr;

        {
            std::shared_lock<std::shared_mutex> lock(internal::g_mesh_cache_mutex);
            if (auto it = internal::g_mesh_cache.find(asset_id); it != internal::g_mesh_cache.end()) {
                return it->second;
            }
        }

        {
            std::lock_guard<std::mutex> lock(internal::g_queue_mutex);
            if (!internal::g_pending_assets[asset_id]) {
                internal::g_pending_assets[asset_id] = true;
                internal::g_task_queue.push(asset_id);
                internal::g_queue_cv.notify_one();
            }
        }

        return nullptr;
    }
}
