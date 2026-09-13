#include "web_menu.hpp"
#include "../core/globals.hpp"
#include "../math/math.hpp"
#include <WebView2.h>
#include <wrl.h>
#include <string>

using namespace Microsoft::WRL;

namespace ui::web_menu {
    static ComPtr<ICoreWebView2Controller> g_controller;
    static ComPtr<ICoreWebView2> g_webview;
    static bool g_ready = false;
    static bool g_dragging = false;
    static bool g_resizing = false;
    static POINT g_drag_offset = { 0, 0 };
    static POINT g_capture_mouse_pos = { 0, 0 };
    static RECT  g_capture_bounds = { 0, 0, 0, 0 };
    static RECT  g_bounds = { 100, 100, 860, 580 };

    auto initialize(HWND overlay_hwnd) -> bool {
        RECT rect;
        if (GetClientRect(overlay_hwnd, &rect)) {
            int sw = rect.right - rect.left;
            int sh = rect.bottom - rect.top;
            int mw = g_bounds.right - g_bounds.left;
            int mh = g_bounds.bottom - g_bounds.top;
            g_bounds.left = (sw - mw) / 2;
            g_bounds.top = (sh - mh) / 2;
            g_bounds.right = g_bounds.left + mw;
            g_bounds.bottom = g_bounds.top + mh;
        }

        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        
        wchar_t temp_path[MAX_PATH];
        GetTempPathW(MAX_PATH, temp_path);
        std::wstring user_data_folder = std::wstring(temp_path) + L"violet_webview";

        CreateCoreWebView2EnvironmentWithOptions(nullptr, user_data_folder.c_str(), nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [overlay_hwnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                    if (FAILED(result)) return result;

                    env->CreateCoreWebView2Controller(overlay_hwnd,
                        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                            [](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                                if (FAILED(result)) return result;

                                g_controller = controller;
                                g_controller->get_CoreWebView2(&g_webview);

                                ComPtr<ICoreWebView2Settings> settings;
                                g_webview->get_Settings(&settings);
                                settings->put_IsScriptEnabled(TRUE);
                                settings->put_AreDefaultScriptDialogsEnabled(FALSE);
                                settings->put_IsWebMessageEnabled(TRUE);
                                settings->put_AreDevToolsEnabled(FALSE);
                                settings->put_AreDefaultContextMenusEnabled(FALSE);
                                settings->put_IsStatusBarEnabled(FALSE);

                                ComPtr<ICoreWebView2Controller2> controller2;
                                if (SUCCEEDED(controller->QueryInterface(IID_PPV_ARGS(&controller2)))) {
                                    COREWEBVIEW2_COLOR transparent = { 0, 0, 0, 0 };
                                    controller2->put_DefaultBackgroundColor(transparent);
                                }

                                g_controller->put_Bounds(g_bounds);
                                g_controller->put_IsVisible(FALSE);

                                std::wstring head = LR"html(
                                    <!DOCTYPE html>
                                    <html>
                                    <head>
                                      <meta charset="UTF-8">
                                      <script src="https://cdn.tailwindcss.com"></script>
                                      <style>
                                        @import url('https://fonts.googleapis.com/css2?family=Sora:wght@300;400;500;600;700;800&display=swap');
                                        :root { --acc: #fff; --orb1: rgba(255,255,255,0.12); --orb2: rgba(255,255,255,0.08); }
                                        * { cursor: default !important; user-select: none !important; -webkit-user-drag: none !important; box-sizing: border-box; }
                                        body { background: transparent !important; margin: 0; padding: 0; overflow: hidden; height: 100vh; width: 100vw; font-family: 'Sora', sans-serif; color: #fff; -webkit-font-smoothing: antialiased; }
                                        .orb { position: fixed; border-radius: 50%; pointer-events: none; transition: all 2s ease; }
                                        .orb1 { width: 300px; height: 300px; background: var(--orb1); filter: blur(80px); top: -10%; left: -10%; animation: drift 40s ease-in-out infinite alternate; z-index: 1; }
                                        .orb2 { width: 350px; height: 350px; background: var(--orb2); filter: blur(100px); bottom: -10%; left: -10%; animation: drift-slow 50s ease-in-out infinite alternate; z-index: 1; }
                                        @keyframes drift { 0% { transform: translate(0,0) scale(1); } 50% { transform: translate(20px,15px) scale(1.1); } 100% { transform: translate(-10px,10px) scale(0.95); } }
                                        @keyframes drift-slow { 0% { transform: translate(0,0) scale(1); } 50% { transform: translate(-15px,-20px) scale(1.1); } 100% { transform: translate(10px,-10px) scale(1); } }
                                        .main-window { position: relative; height: 100vh; width: 100vw; margin: 0; background: #000; border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 8px; overflow: hidden; transition: opacity 0.15s ease-out; opacity: 0; }
                                        .main-window.active { opacity: 1; }
                                        .glass-overlay { position: absolute; inset: 0; z-index: 10; display: flex; flex-direction: column; align-items: center; justify-content: center; background: transparent; }
                                        .login-card { width: 270px; display: flex; flex-direction: column; gap: 18px; position: relative; z-index: 20; }
                                      </style>
                                )html"
                                + std::wstring(L" ")
                                + LR"html(
                                      <style>
                                        .login-title { font-size: 24px; font-weight: 700; text-align: center; letter-spacing: -0.03em; margin-bottom: 4px; }
                                        .input-group { display: flex; flex-direction: column; gap: 6px; }
                                        .input-group label { font-size: 11px; font-weight: 600; color: rgba(255,255,255,.4); margin-left: 2px; }
                                        .v-input { width: 100%; background: rgba(255,255,255,.02); border: 1px solid rgba(255,255,255,0.06); border-radius: 8px; padding: 10px 14px; font-size: 13px; color: #fff; outline: none; transition: all .2s; font-family: 'Sora', sans-serif; cursor: default !important; }
                                        .v-input::-ms-reveal, .v-input::-ms-clear { display: none; }
                                        .v-input:focus { border-color: rgba(255,255,255,0.2); background: rgba(255,255,255,.04); }
                                        .v-btn { width: 100%; padding: 11px; border-radius: 8px; background: #fff; color: #000; font-size: 13px; font-weight: 700; border: none; cursor: default !important; transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1); position: relative; overflow: hidden; }
                                        .v-btn:hover { opacity: .9; transform: translateY(-1px); box-shadow: 0 4px 15px rgba(255,255,255,0.1); }
                                        .v-btn:active { transform: scale(0.92); }
                                        .v-btn.loading { pointer-events: none; opacity: 0.8; }
                                        .v-btn.loading .btn-text { display: none; }
                                        .spinner { width: 18px; height: 18px; border: 2.5px solid rgba(0,0,0,0.15); border-top-color: #000; border-radius: 50%; animation: spin 0.8s cubic-bezier(0.4, 0, 0.2, 1) infinite; display: none; margin: 0 auto; }
                                        @keyframes spin { to { transform: rotate(360deg); } }
                                        .v-btn.loading .spinner { display: block; }
                                        .remember-me { color: rgba(255,255,255,.25); font-size: 11px; font-weight: 500; display: flex; align-items: center; gap: 8px; cursor: default !important; user-select: none; transition: color 0.2s; }
                                        .remember-me:hover { color: rgba(255,255,255,.5); }
                                        .remember-me input { appearance: none; width: 14px; height: 14px; border: 1px solid rgba(255,255,255,0.08); border-radius: 4px; background: rgba(255,255,255,0.02); cursor: default !important; position: relative; transition: all 0.2s; }
                                        .remember-me input:checked { background: #fff; border-color: #fff; }
                                        .remember-me input:checked::after { content: ''; position: absolute; left: 4px; top: 1px; width: 4px; height: 8px; border: solid black; border-width: 0 2.2px 2.2px 0; transform: rotate(45deg); }
                                        .nav-pill { padding: 6px 15px; border-radius: 9999px; font-size: 11px; font-weight: 700; color: rgba(255,255,255,0.4); transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1); cursor: default !important; white-space: nowrap; }
                                        .nav-pill:hover { color: rgba(255,255,255,0.8); }
                                        .nav-pill.active { color: #000; background: #fff; box-shadow: 0 4px 12px rgba(255,255,255,0.1); }
                                        .nav-bar { width: fit-content; padding: 4px; background: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.06); border-radius: 9999px; backdrop-blur: 12px; display: flex; gap: 2px; }
                                        .v-card { background: rgba(255,255,255,0.02); border: 1px solid rgba(255,255,255,0.05); border-radius: 20px; backdrop-blur: 12px; transition: all 0.3s; overflow: hidden; }
                                        .v-card:hover { border-color: rgba(255,255,255,0.1); background: rgba(255,255,255,0.03); }
                                        .resize-grip { position: absolute; bottom: 0; right: 0; width: 24px; height: 24px; cursor: default !important; z-index: 100; }
                                        .v-dropdown-menu.active { opacity: 1; pointer-events: auto; transform: translateY(0); }
                                        .v-dropdown.open svg { transform: rotate(180deg); }
                                      </style>
                                )html"
                                + std::wstring(L" ")
                                + LR"html(
                                      <script>
                                        window.addEventListener('keydown', e => {
                                          if ((e.ctrlKey || e.metaKey) && (e.key === 'a' || e.key === 'c' || e.key === 'v')) {
                                            e.preventDefault();
                                          }
                                        });

                                        function setVisible(v) {
                                          const app = document.getElementById('app');
                                          if (!app) return;
                                          if (v) {
                                            app.classList.add('active');
                                          } else {
                                            app.classList.remove('active');
                                          }
                                        }

                                        function doLogin() {
                                          const email = document.getElementById('emailInput').value;
                                          const password = document.getElementById('passwordInput').value;
                                          const btn = document.getElementById('loginBtn');
                                          
                                          if (!email.includes('@') || password.length < 4) {
                                            btn.style.background = '#ff4444';
                                            btn.style.color = '#fff';
                                            setTimeout(() => { btn.style.background = '#fff'; btn.style.color = '#000'; }, 500);
                                            return;
                                          }

                                          if (btn.classList.contains('loading')) return;
                                          btn.classList.add('loading');
                                          
                                          setTimeout(() => {
                                            btn.classList.remove('loading');
                                            document.getElementById('loginPage').classList.add('opacity-0', 'scale-95', 'pointer-events-none');
                                            setTimeout(() => {
                                              document.getElementById('loginPage').classList.add('hidden');
                                              const main = document.getElementById('mainMenu');
                                              main.classList.remove('hidden');
                                              setTimeout(() => {
                                                main.classList.remove('opacity-0');
                                                switchTab('Aiming');
                                              }, 50);
                                            }, 500);
                                          }, 1500);
                                        }

                                          function toggleDropdown(el) {
                                            const menu = el.nextElementSibling;
                                            const isOpen = el.classList.contains('open');
                                            document.querySelectorAll('.v-dropdown.open').forEach(d => {
                                              if (d !== el) {
                                                d.classList.remove('open');
                                                d.nextElementSibling.classList.remove('active');
                                              }
                                            });
                                            if (isOpen) {
                                              el.classList.remove('open');
                                              menu.classList.remove('active');
                                            } else {
                                              el.classList.add('open');
                                              menu.classList.add('active');
                                            }
                                          }

                                          function selectDropdownItem(el, text) {
                                            const menu = el.closest('.v-dropdown-menu');
                                            const dropdown = menu.previousElementSibling;
                                            dropdown.querySelector('span').innerText = text;
                                            dropdown.classList.remove('open');
                                            menu.classList.remove('active');
                                            menu.querySelectorAll('.v-dropdown-item').forEach(item => {
                                              if (item === el) {
                                                item.classList.add('text-white', 'bg-white/5');
                                                item.classList.remove('text-white/60');
                                              } else {
                                                item.classList.remove('text-white', 'bg-white/5');
                                                item.classList.add('text-white/60');
                                              }
                                            });
                                            
                                            if (text === 'Aimbot' || text === 'Silent Aim') {
                                              const aimbotGroup = document.getElementById('aimbot-settings-group');
                                              const silentGroup = document.getElementById('silentaim-settings-group');
                                              if (aimbotGroup && silentGroup) {
                                                if (text === 'Aimbot') {
                                                  aimbotGroup.classList.remove('hidden');
                                                  silentGroup.classList.add('hidden');
                                                } else {
                                                  aimbotGroup.classList.add('hidden');
                                                  silentGroup.classList.remove('hidden');
                                                }
                                              }
                                            }
                                          }

                                          document.addEventListener('click', (e) => {
                                            if (!e.target.closest('.relative')) {
                                              document.querySelectorAll('.v-dropdown.open').forEach(d => {
                                                d.classList.remove('open');
                                                d.nextElementSibling.classList.remove('active');
                                              });
                                            }
                                          });

                                )html"
                                + std::wstring(L" ")
                                + LR"html(
                                        function switchTab(name) {
                                          const tabs = document.querySelectorAll('.nav-pill');
                                          tabs.forEach(t => {
                                            if (t.innerText.trim() === name) t.classList.add('active');
                                            else t.classList.remove('active');
                                          });
                                          const content = document.getElementById('tabContent');
                                          if (content) {
                                            content.style.opacity = '0';
                                            setTimeout(() => {
                                              if (name === 'Aiming') {
                                                content.innerHTML = `
                                                  <div class="h-full w-full px-3 pb-3 pt-2">
                                                    <div class="v-card h-full w-full flex">
                                                       <div class="flex-1 flex flex-col">
                                                         <div class="px-5 border-b border-white/5 flex items-center min-h-[50px]">
                                                           <div class="text-[11px] font-semibold text-white/30 select-none">Aiming Feature</div>
                                                         </div>
                                                         <div class="flex-1 p-5 flex flex-col gap-5">
                                                           <div class="relative w-full z-30">
                                                             <label class="text-[11px] font-semibold text-white/40 mb-1.5 block">Select Feature</label>
                                                             <div class="v-dropdown bg-white/5 border border-white/10 rounded-lg px-3 py-2 flex items-center justify-between cursor-pointer transition-all hover:bg-white/10" onclick="toggleDropdown(this)">
                                                               <span class="text-[12px] text-white/80 font-medium">Aimbot</span>
                                                               <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" class="text-white/40 transition-transform duration-200"><path d="M6 9l6 6 6-6"/></svg>
                                                             </div>
                                                              <div class="v-dropdown-menu absolute top-[calc(100%+4px)] left-0 w-full bg-[#0a0a0a] border border-white/10 rounded-lg overflow-hidden opacity-0 pointer-events-none transition-all duration-200 transform translate-y-[-4px] shadow-xl">
                                                                <div class="v-dropdown-item px-3 py-2 text-[12px] text-white bg-white/5 cursor-pointer transition-colors" onclick="selectDropdownItem(this, 'Aimbot')">Aimbot</div>
                                                                <div class="v-dropdown-item px-3 py-2 text-[12px] text-white/60 hover:text-white hover:bg-white/5 cursor-pointer transition-colors" onclick="selectDropdownItem(this, 'Silent Aim')">Silent Aim</div>
                                                              </div>
                                                            </div>
                                )html"
                                + std::wstring(L" ")
                                + LR"html(
                                                            <div id="aimbot-settings-group" class="flex flex-col gap-4 w-full">
                                                             <div class="relative w-full z-20">
                                                               <label class="text-[11px] font-semibold text-white/40 mb-1.5 block">Aimbot Mode</label>
                                                               <div class="v-dropdown bg-white/5 border border-white/10 rounded-lg px-3 py-2 flex items-center justify-between cursor-pointer transition-all hover:bg-white/10" onclick="toggleDropdown(this)">
                                                                 <span class="text-[12px] text-white/80 font-medium">Mouse</span>
                                                                 <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" class="text-white/40 transition-transform duration-200"><path d="M6 9l6 6 6-6"/></svg>
                                                               </div>
                                                               <div class="v-dropdown-menu absolute top-[calc(100%+4px)] left-0 w-full bg-[#0a0a0a] border border-white/10 rounded-lg overflow-hidden opacity-0 pointer-events-none transition-all duration-200 transform translate-y-[-4px] shadow-xl">
                                                                 <div class="v-dropdown-item px-3 py-2 text-[12px] text-white bg-white/5 cursor-pointer transition-colors" onclick="selectDropdownItem(this, 'Mouse')">Mouse</div>
                                                                 <div class="v-dropdown-item px-3 py-2 text-[12px] text-white/60 hover:text-white hover:bg-white/5 cursor-pointer transition-colors" onclick="selectDropdownItem(this, 'Memory')">Memory</div>
                                                               </div>
                                                             </div>
                                                           </div>
                                                           <div id="silentaim-settings-group" class="flex flex-col gap-4 w-full hidden">
                                                             <div class="relative w-full z-20">
                                                               <label class="text-[11px] font-semibold text-white/40 mb-1.5 block">Silent Aim Mode</label>
                                                               <div class="v-dropdown bg-white/5 border border-white/10 rounded-lg px-3 py-2 flex items-center justify-between cursor-pointer transition-all hover:bg-white/10" onclick="toggleDropdown(this)">
                                                                 <span class="text-[12px] text-white/80 font-medium">New</span>
                                                                 <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" class="text-white/40 transition-transform duration-200"><path d="M6 9l6 6 6-6"/></svg>
                                                               </div>
                                                               <div class="v-dropdown-menu absolute top-[calc(100%+4px)] left-0 w-full bg-[#0a0a0a] border border-white/10 rounded-lg overflow-hidden opacity-0 pointer-events-none transition-all duration-200 transform translate-y-[-4px] shadow-xl">
                                                                 <div class="v-dropdown-item px-3 py-2 text-[12px] text-white bg-white/5 cursor-pointer transition-colors" onclick="selectDropdownItem(this, 'New')">New</div>
                                                                 <div class="v-dropdown-item px-3 py-2 text-[12px] text-white/60 hover:text-white hover:bg-white/5 cursor-pointer transition-colors" onclick="selectDropdownItem(this, 'Experimental')">Experimental</div>
                                                               </div>
                                                             </div>
                                                           </div>
                                                         </div>
                                                       </div>
                                                       <div class="w-[1px] h-full bg-white/5"></div>
                                                       <div class="flex-1 flex flex-col">
                                                         <div class="px-5 border-b border-white/5 flex items-center min-h-[50px]">
                                                           <div class="text-[11px] font-semibold text-white/30 select-none">Settings</div>
                                                         </div>
                                                         <div class="flex-1 p-5 flex flex-col"></div>
                                                       </div>
                                                    </div>
                                                  </div>
                                                `;
                                              } else {
                                                content.innerHTML = '';
                                              }
                                              content.style.opacity = '1';
                                            }, 150);
                                          }
                                        }
                                      </script>
                                    </head>
                                )html";

                                std::wstring body = LR"html(
                                    <body>
                                      <div id="app" class="main-window">
                                        <div class="orb orb1"></div>
                                        <div class="orb orb2"></div>
                                        <div class="glass-overlay">
                                          <div id="loginPage" class="login-card transition-all duration-500">
                                )html"
                                + std::wstring(L" ")
                                + LR"html(
                                            <div class="login-title">Welcome back</div>
                                            <div class="space-y-4">
                                              <div class="input-group">
                                                <label>Email</label>
                                                <input id="emailInput" class="v-input" type="text" placeholder="user@violet.app">
                                              </div>
                                )html"
                                + std::wstring(L" ")
                                + LR"html(
                                              <div class="input-group">
                                                <label>Password</label>
                                                <input id="passwordInput" class="v-input" type="password" placeholder="********">
                                              </div>
                                              <div class="flex items-center pt-1">
                                                <label class="remember-me">
                                                  <input type="checkbox">
                                                  Remember Me?
                                                </label>
                                              </div>
                                              <div class="pt-2">
                                                <button id="loginBtn" class="v-btn" onclick="doLogin()">
                                                  <span class="btn-text">Sign in</span>
                                                  <div class="spinner"></div>
                                                </button>
                                              </div>
                                            </div>
                                           </div>
                                 )html";
                                 std::wstring body2 = LR"html(
                                            <div id="mainMenu" class="hidden opacity-0 transition-all duration-500 h-full w-full flex flex-col">
                                             <div class="w-full pt-3 px-3 flex items-center">
                                               <div class="nav-bar flex-none">
                                                 <div class="nav-pill active" onclick="switchTab('Aiming')">Aiming</div>
                                                 <div class="nav-pill" onclick="switchTab('Visuals')">Visuals</div>
                                                 <div class="nav-pill" onclick="switchTab('Character')">Character</div>
                                                 <div class="nav-pill" onclick="switchTab('World')">World</div>
                                                 <div class="nav-pill" onclick="switchTab('Rivals')">Rivals</div>
                                                 <div class="nav-pill" onclick="switchTab('Settings')">Settings</div>
                                                 <div class="nav-pill" onclick="switchTab('Config')">Config</div>
                                                 <div class="nav-pill" onclick="switchTab('Account')">Account</div>
                                               </div>
                                               <div class="flex-1 flex justify-center pointer-events-none select-none">
                                                 <div class="text-center">
                                                   <div class="text-white font-bold text-sm tracking-widest">Violet :3</div>
                                )html"
                                + (globals::devbuild.load() ? std::wstring(L"<div class=\"text-white/40 text-[10px] font-medium tracking-wide mt-0.5\">Developer Build</div>") : L"") +
                                LR"html(
                                                 </div>
                                               </div>
                                             </div>
                                             <div id="tabContent" class="flex-1 transition-all duration-300"></div>
                                          </div>
                                          <div class="resize-grip"></div>
                                        </div>
                                      </body>
                                    </html>
                                )html";

                                g_webview->NavigateToString((head + body + body2).c_str());
                                g_ready = true;
                                return S_OK;
                            }).Get());
                    return S_OK;
                }).Get());

        return true;
    }

    auto shutdown() -> void {
        if (g_controller) {
            g_controller->Close();
            g_controller = nullptr;
            g_webview = nullptr;
        }
        CoUninitialize();
    }

    static auto show() -> void {
        if (g_ready && g_controller) {
            g_controller->put_IsVisible(TRUE);
            g_webview->ExecuteScript(L"if(typeof setVisible === 'function') setVisible(true); else setTimeout(() => setVisible(true), 50);", nullptr);
        }
    }

    auto is_open() -> bool {
        return globals::menu_open.load();
    }

    auto is_ready() -> bool {
        return g_ready;
    }

    auto process_input(HWND overlay_hwnd, HWND game_hwnd, bool allowed) -> void {
        static bool last_visible_state = false;
        static bool is_fading_out = false;
        static ULONGLONG fade_start_time = 0;

        bool menu_open = globals::menu_open.load();

        if (allowed && (GetAsyncKeyState(globals::settings::menu_keybind) & 1)) {
            menu_open = !menu_open;
            globals::menu_open.store(menu_open);
        }

        bool should_be_visible = allowed && menu_open;

        if (should_be_visible != last_visible_state) {
            if (should_be_visible) {
                show();
                last_visible_state = true;
                is_fading_out = false;
            }
            else if (!is_fading_out) {
                if (g_ready && g_webview) {
                    g_webview->ExecuteScript(L"setVisible(false)", nullptr);
                }
                fade_start_time = GetTickCount64();
                is_fading_out = true;
            }
        }

        if (is_fading_out) {
            if (GetTickCount64() - fade_start_time >= 150) {
                if (g_controller) g_controller->put_IsVisible(FALSE);
                g_dragging = false;
                g_resizing = false;
                last_visible_state = false;
                is_fading_out = false;
            }
        }

        if (!should_be_visible && !is_fading_out) return;
        if (!g_ready || !g_controller) return;

        Vector2 viewport = math::get_viewport_size();
        if (viewport.x <= 0 || viewport.y <= 0) {
            RECT rect;
            if (GetClientRect(overlay_hwnd, &rect)) {
                viewport.x = static_cast<float>(rect.right - rect.left);
                viewport.y = static_cast<float>(rect.bottom - rect.top);
            }
        }

        POINT mouse_pos;
        if (!GetCursorPos(&mouse_pos)) return;
        ScreenToClient(overlay_hwnd, &mouse_pos);

        bool l_button = (GetAsyncKeyState(VK_LBUTTON) & 0x8000);

        if (l_button) {
            if (!g_dragging && !g_resizing) {
                if (mouse_pos.x >= g_bounds.right - 24 && mouse_pos.x <= g_bounds.right + 4 &&
                    mouse_pos.y >= g_bounds.bottom - 24 && mouse_pos.y <= g_bounds.bottom + 4) {
                    g_resizing = true;
                    g_capture_mouse_pos = mouse_pos;
                    g_capture_bounds = g_bounds;
                }
                else if (mouse_pos.x >= g_bounds.left && mouse_pos.x <= g_bounds.right &&
                         mouse_pos.y >= g_bounds.top && mouse_pos.y <= g_bounds.bottom) {
                    g_dragging = true;
                    g_drag_offset.x = mouse_pos.x - g_bounds.left;
                    g_drag_offset.y = mouse_pos.y - g_bounds.top;
                }
            } else if (g_dragging) {
                int width = g_bounds.right - g_bounds.left;
                int height = g_bounds.bottom - g_bounds.top;
                
                int next_left = mouse_pos.x - g_drag_offset.x;
                int next_top = mouse_pos.y - g_drag_offset.y;
                
                if (next_left < 0) next_left = 0;
                if (next_top < 0) next_top = 0;
                if (next_left + width > (int)viewport.x) next_left = (int)viewport.x - width;
                if (next_top + height > (int)viewport.y) next_top = (int)viewport.y - height;
                
                if (next_left != g_bounds.left || next_top != g_bounds.top) {
                    g_bounds.left = next_left;
                    g_bounds.top = next_top;
                    g_bounds.right = g_bounds.left + width;
                    g_bounds.bottom = g_bounds.top + height;
                    
                    g_controller->put_Bounds(g_bounds);
                }
            } else if (g_resizing) {
                int dx = mouse_pos.x - g_capture_mouse_pos.x;
                int dy = mouse_pos.y - g_capture_mouse_pos.y;

                RECT next_bounds = g_capture_bounds;
                next_bounds.right = g_capture_bounds.right + dx;
                next_bounds.bottom = g_capture_bounds.bottom + dy;
                
                if (next_bounds.right - next_bounds.left < 760) next_bounds.right = next_bounds.left + 760;
                if (next_bounds.bottom - next_bounds.top < 480) next_bounds.bottom = next_bounds.top + 480;
                
                if (next_bounds.right > (int)viewport.x) next_bounds.right = (int)viewport.x;
                if (next_bounds.bottom > (int)viewport.y) next_bounds.bottom = (int)viewport.y;
                
                if (next_bounds.right != g_bounds.right || next_bounds.bottom != g_bounds.bottom) {
                    g_bounds = next_bounds;
                    
                    static ULONGLONG last_update = 0;
                    ULONGLONG now = GetTickCount64();
                    if (now - last_update > 8) {
                        g_controller->put_Bounds(g_bounds);
                        last_update = now;
                    }
                }
            }
        } else {
            if (g_dragging || g_resizing) {
                g_controller->put_Bounds(g_bounds);
            }
            g_dragging = false;
            g_resizing = false;
        }
    }
}
