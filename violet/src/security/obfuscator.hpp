#ifndef OBFUSCATOR_HPP
#define OBFUSCATOR_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <cstddef>

namespace enc {

    namespace detail {

        constexpr uint64_t fnv1a(const char* s) {
            uint64_t h = 0xcbf29ce484222325ULL;
            while (*s) {
                h ^= static_cast<uint64_t>(*s++);
                h *= 0x100000001b3ULL;
            }
            return h;
        }

        constexpr uint64_t splitmix64_finalize(uint64_t x) {
            x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
            x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
            x ^= (x >> 31);
            return x;
        }

        constexpr uint64_t seed64_from(const char* file, int line, const char* t) {
            uint64_t h = fnv1a(file);
            h ^= static_cast<uint64_t>(line) * 0x9E3779B97F4A7C15ULL;
            h ^= fnv1a(t) * 0xBF58476D1CE4E5B9ULL;
            return splitmix64_finalize(h);
        }

        constexpr uint64_t round_key(uint64_t seed, size_t i, int r) {
            uint64_t s = seed;
            s += 0x9E3779B97F4A7C15ULL * (static_cast<uint64_t>(i) + 1ULL);
            s ^= 0xBF58476D1CE4E5B9ULL * (static_cast<uint64_t>(r + 1));
            return splitmix64_finalize(s);
        }

        constexpr uint8_t mod_inv8(uint8_t a) {
            uint32_t x = 1u;
            for (int k = 0; k < 8; ++k) {
                x = (x * (2u - (static_cast<uint32_t>(a) * x))) & 0xFFu;
            }
            return static_cast<uint8_t>(x);
        }

        constexpr uint8_t MUL1 = 0xADu, ADD1 = 0x47u;
        constexpr uint8_t MUL2 = 0x53u, ADD2 = 0x91u;

        constexpr uint8_t INV_MUL1 = mod_inv8(MUL1);
        constexpr uint8_t INV_MUL2 = mod_inv8(MUL2);

        constexpr uint8_t rotl8(uint8_t b, int n) {
            return static_cast<uint8_t>((b << n) | (b >> (8 - n)));
        }
        constexpr uint8_t rotr8(uint8_t b, int n) {
            return static_cast<uint8_t>((b >> n) | (b << (8 - n)));
        }

        constexpr uint8_t round_fwd(uint8_t b, uint64_t rk) {
            b ^= static_cast<uint8_t>(rk);
            b  = rotl8(b, 3);
            b  = static_cast<uint8_t>(b * MUL1 + ADD1);
            b ^= static_cast<uint8_t>(rk >> 16);
            b  = rotl8(b, 5);
            b  = static_cast<uint8_t>(b * MUL2 + ADD2);
            b ^= static_cast<uint8_t>(rk >> 32);
            b  = rotl8(b, 1);
            b ^= static_cast<uint8_t>(rk >> 48);
            return b;
        }

        constexpr uint8_t round_inv(uint8_t b, uint64_t rk) {
            b ^= static_cast<uint8_t>(rk >> 48);
            b  = rotr8(b, 1);
            b ^= static_cast<uint8_t>(rk >> 32);
            b  = static_cast<uint8_t>(static_cast<uint8_t>(b - ADD2) * INV_MUL2);
            b  = rotr8(b, 5);
            b ^= static_cast<uint8_t>(rk >> 16);
            b  = static_cast<uint8_t>(static_cast<uint8_t>(b - ADD1) * INV_MUL1);
            b  = rotr8(b, 3);
            b ^= static_cast<uint8_t>(rk);
            return b;
        }

        constexpr int ROUNDS = 6;
        static_assert(ROUNDS >= 1, "ROUNDS must be positive");

        constexpr uint64_t HI_MASK = 0xA5A5A5A5A5A5A5A5ULL;

        template<size_t N>
        struct encrypted_data {
            uint64_t seed;
            uint8_t  iv;
            uint8_t  data[N];
            size_t   len;
        };

        template<size_t N>
        struct encrypted_wdata {
            uint64_t seed;
            uint16_t iv;
            uint16_t data[N];
            size_t   len;
        };

        template<size_t N>
        consteval encrypted_data<N> encrypt(const char (&str)[N], uint64_t seed) {
            encrypted_data<N> result{};
            result.seed = seed;
            result.len  = N;
            const uint8_t iv = static_cast<uint8_t>(round_key(seed, 0, -1) >> 23);
            result.iv = iv;

            uint8_t prev = iv;
            for (size_t i = 0; i < N; ++i) {
                uint8_t b = static_cast<uint8_t>(str[i]);
                b ^= prev;
                for (int r = 0; r < ROUNDS; ++r) {
                    b = round_fwd(b, round_key(seed, i, r));
                }
                result.data[i] = b;
                prev = b;
            }
            return result;
        }

        template<size_t N>
        consteval encrypted_wdata<N> encrypt_w(const wchar_t (&str)[N], uint64_t seed) {
            encrypted_wdata<N> result{};
            result.seed = seed;
            result.len  = N;
            const uint16_t iv = static_cast<uint16_t>(round_key(seed, 0, -1) >> 16);
            result.iv = iv;

            uint16_t prev = iv;
            for (size_t i = 0; i < N; ++i) {
                uint16_t w  = static_cast<uint16_t>(str[i]) ^ prev;
                uint8_t  lo = static_cast<uint8_t>(w & 0xFFu);
                uint8_t  hi = static_cast<uint8_t>(w >> 8);
                for (int r = 0; r < ROUNDS; ++r) {
                    const uint64_t k = round_key(seed, i, r);
                    lo = round_fwd(lo, k);
                    hi = round_fwd(hi, k ^ HI_MASK);
                }
                const uint16_t c = static_cast<uint16_t>(lo)
                                 | static_cast<uint16_t>(static_cast<uint16_t>(hi) << 8);
                result.data[i] = c;
                prev = c;
            }
            return result;
        }

        template<size_t N>
        class decrypted_string {
        public:
            __forceinline decrypted_string(const encrypted_data<N>& enc) {
                const uint64_t seed = enc.seed;
                uint8_t prev = enc.iv;
                for (size_t i = 0; i < N; ++i) {
                    const uint8_t c = enc.data[i];
                    uint8_t b = c;
                    for (int r = ROUNDS - 1; r >= 0; --r) {
                        b = round_inv(b, round_key(seed, i, r));
                    }
                    b ^= prev;
                    buf[i] = static_cast<char>(b);
                    prev = c;
                }
            }

            __forceinline ~decrypted_string() {
                volatile char* p = buf;
                for (size_t i = 0; i < N; ++i) p[i] = 0;
            }

            __forceinline const char* c_str() const { return buf; }
            __forceinline operator const char*() const { return buf; }

        private:
            char buf[N]{};
        };

        template<size_t N>
        class decrypted_wstring {
        public:
            __forceinline decrypted_wstring(const encrypted_wdata<N>& enc) {
                const uint64_t seed = enc.seed;
                uint16_t prev = enc.iv;
                for (size_t i = 0; i < N; ++i) {
                    const uint16_t c = enc.data[i];
                    uint8_t lo = static_cast<uint8_t>(c & 0xFFu);
                    uint8_t hi = static_cast<uint8_t>(c >> 8);
                    for (int r = ROUNDS - 1; r >= 0; --r) {
                        const uint64_t k = round_key(seed, i, r);
                        hi = round_inv(hi, k ^ HI_MASK);
                        lo = round_inv(lo, k);
                    }
                    uint16_t w = static_cast<uint16_t>(lo)
                               | static_cast<uint16_t>(static_cast<uint16_t>(hi) << 8);
                    w ^= prev;
                    buf[i] = static_cast<wchar_t>(w);
                    prev = c;
                }
            }

            __forceinline ~decrypted_wstring() {
                volatile wchar_t* p = buf;
                for (size_t i = 0; i < N; ++i) p[i] = 0;
            }

            __forceinline const wchar_t* c_str() const { return buf; }
            __forceinline operator const wchar_t*() const { return buf; }

        private:
            wchar_t buf[N]{};
        };

    }

}

#define ENC_SEED enc::detail::seed64_from(__FILE__, __LINE__, __TIME__)

#define HIDE_STR(str) ([]() -> const std::string& { \
    static const std::string decrypted = []() { \
        constexpr auto _enc = enc::detail::encrypt(str, ENC_SEED); \
        enc::detail::decrypted_string<sizeof(str)> _dec(_enc); \
        return std::string(_dec.c_str()); \
    }(); \
    return decrypted; \
}())

#define HIDE_WSTR(str) ([]() -> const std::wstring& { \
    static const std::wstring decrypted = []() { \
        constexpr auto _enc = enc::detail::encrypt_w(str, ENC_SEED); \
        enc::detail::decrypted_wstring<sizeof(str)/sizeof(wchar_t)> _dec(_enc); \
        return std::wstring(_dec.c_str()); \
    }(); \
    return decrypted; \
}())

namespace Crypto {
    class XorGuard {
    private:
        std::vector<uint8_t> key_bytes;

    public:
        XorGuard() = delete;
        explicit XorGuard(const std::string& key) {
            if (key.empty()) throw std::invalid_argument("Key cannot be empty.");
            key_bytes.assign(key.begin(), key.end());
        }

        explicit XorGuard(const std::vector<uint8_t>& key) : key_bytes(key) {
            if (key.empty()) throw std::invalid_argument("Key cannot be empty.");
        }

        template <typename T>
        void process_inplace(T& data) const {
            if (data.empty()) return;
            size_t key_len = key_bytes.size();
            for (size_t i = 0; i < data.size(); ++i) {
                data[i] = static_cast<typename T::value_type>(
                    static_cast<uint8_t>(data[i]) ^ key_bytes[i % key_len]
                    );
            }
        }
        template <typename T>
        T process_copy(const T& data) const {
            T copy = data;
            process_inplace(copy);
            return copy;
        }

        void process_buffer(uint8_t* ptr, size_t size) const {
            if (!ptr || size == 0) return;
            size_t key_len = key_bytes.size();
            for (size_t i = 0; i < size; ++i) ptr[i] ^= key_bytes[i % key_len];
        }
    };
}

#endif
