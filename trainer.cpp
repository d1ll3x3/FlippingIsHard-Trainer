#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include <fstream>
#include <mutex>
#include <atomic>

// ---- Overlay HUD state ----
std::atomic<bool> g_overlayRunning(false);
HWND             g_overlayHwnd = nullptr;
std::atomic<bool> g_hasSavedPosOverlay(false); // mirrors g_hasSavedPos for the overlay thread

// Structs for Unity types
struct Vector3 {
    float x, y, z;
};
struct Vector2 {
    float x, y;
};
struct Quaternion {
    float x, y, z, w;
};

// IL2CPP Core Export Signatures
typedef void* (*t_il2cpp_domain_get)();
typedef void* (*t_il2cpp_thread_attach)(void* domain);
typedef void* (*t_il2cpp_resolve_icall)(const char* name);
typedef void* (*t_il2cpp_string_new)(const char* str);

// IL2CPP Metadata Export Signatures
typedef void** (*t_il2cpp_domain_get_assemblies)(void* domain, size_t* size);
typedef void* (*t_il2cpp_assembly_get_image)(void* assembly);
typedef const char* (*t_il2cpp_image_get_name)(void* image);
typedef void* (*t_il2cpp_class_from_name)(void* image, const char* namespaze, const char* name);
typedef void* (*t_il2cpp_class_get_methods)(void* klass, void** iter);
typedef const char* (*t_il2cpp_method_get_name)(void* method);
typedef int (*t_il2cpp_method_get_param_count)(void* method);
typedef void* (*t_il2cpp_method_get_param)(void* method, int index);
typedef const char* (*t_il2cpp_type_get_name)(void* type);
typedef void* (*t_il2cpp_class_get_method_from_name)(void* klass, const char* name, int argsCount);
typedef void* (*t_il2cpp_runtime_invoke)(void* method, void* obj, void** params, void** exc);
typedef void* (*t_il2cpp_class_get_type)(void* klass);
typedef void* (*t_il2cpp_type_get_object)(void* type);

// Unity Direct Icall Signatures (resolved successfully via icall on user PC)
typedef void (*t_Transform_get_position)(void* transform, Vector3* out_pos);
typedef void (*t_Transform_set_position)(void* transform, Vector3* in_pos);
typedef void (*t_Transform_get_rotation)(void* transform, Quaternion* out_rot);
typedef void (*t_Transform_set_rotation)(void* transform, Quaternion* in_rot);

// Global IL2CPP core pointers
t_il2cpp_domain_get il2cpp_domain_get_fn = nullptr;
t_il2cpp_thread_attach il2cpp_thread_attach_fn = nullptr;
t_il2cpp_resolve_icall il2cpp_resolve_icall_fn = nullptr;
t_il2cpp_string_new il2cpp_string_new_fn = nullptr;

// Global IL2CPP metadata pointers
t_il2cpp_domain_get_assemblies il2cpp_domain_get_assemblies_fn = nullptr;
t_il2cpp_assembly_get_image il2cpp_assembly_get_image_fn = nullptr;
t_il2cpp_image_get_name il2cpp_image_get_name_fn = nullptr;
t_il2cpp_class_from_name il2cpp_class_from_name_fn = nullptr;
t_il2cpp_class_get_methods il2cpp_class_get_methods_fn = nullptr;
t_il2cpp_method_get_name il2cpp_method_get_name_fn = nullptr;
t_il2cpp_method_get_param_count il2cpp_method_get_param_count_fn = nullptr;
t_il2cpp_method_get_param il2cpp_method_get_param_fn = nullptr;
t_il2cpp_type_get_name il2cpp_type_get_name_fn = nullptr;
t_il2cpp_class_get_method_from_name il2cpp_class_get_method_from_name_fn = nullptr;
t_il2cpp_runtime_invoke il2cpp_runtime_invoke_fn = nullptr;
t_il2cpp_class_get_type il2cpp_class_get_type_fn = nullptr;
t_il2cpp_type_get_object il2cpp_type_get_object_fn = nullptr;

// Unity Direct Transform pointers (icall-based)
t_Transform_get_position Transform_get_position_fn = nullptr;
t_Transform_set_position Transform_set_position_fn = nullptr;
t_Transform_get_rotation Transform_get_rotation_fn = nullptr;
t_Transform_set_rotation Transform_set_rotation_fn = nullptr;

// Metada classes and methods pointers
void* coreImage = nullptr;
void* physicsImage = nullptr;
void* physics2DImage = nullptr;

void* gameObjectClass = nullptr;
void* transformClass = nullptr;
void* rbClass = nullptr;
void* rb2DClass = nullptr;

void* GameObject_Find_Method = nullptr;
void* GameObject_FindWithTag_Method = nullptr;
void* GameObject_get_transform_Method = nullptr;
void* GameObject_GetComponent_Method = nullptr;

void* Rigidbody_set_velocity_Method = nullptr;
void* Rigidbody_set_angularVelocity_Method = nullptr;

void* Rigidbody2D_set_velocity_Method = nullptr;
void* Rigidbody2D_set_angularVelocity_Method = nullptr;

// State variables
bool g_hasSavedPos = false;
Vector3 g_savedPos = { 0.0f, 0.0f, 0.0f };
Quaternion g_savedRot = { 0.0f, 0.0f, 0.0f, 1.0f };

// ---- Overlay rendering ----
static void PaintOverlay(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    // Full transparent background
    RECT rc;
    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

    // Choose a nice font
    HFONT hFont = CreateFontA(
        22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    SetBkMode(hdc, TRANSPARENT);

    // Header
    SetTextColor(hdc, RGB(255, 220, 0)); // gold
    const char* header = "[Trainer]";
    TextOutA(hdc, 10, 8, header, (int)strlen(header));

    // Shortcut 1 – always shown
    SetTextColor(hdc, RGB(100, 220, 255)); // light blue
    const char* line1 = "Shift+R  ->  Save position";
    TextOutA(hdc, 10, 34, line1, (int)strlen(line1));

    // Shortcut 2 – green when position is saved, grey otherwise
    if (g_hasSavedPosOverlay.load()) {
        SetTextColor(hdc, RGB(80, 255, 120)); // bright green
    } else {
        SetTextColor(hdc, RGB(150, 150, 150)); // grey (not saved yet)
    }
    const char* line2 = "R        ->  Teleport";
    TextOutA(hdc, 10, 58, line2, (int)strlen(line2));

    // Hint when no position saved
    if (!g_hasSavedPosOverlay.load()) {
        SetTextColor(hdc, RGB(200, 80, 80)); // red hint
        const char* hint = "(Save first with Shift+R)";
        TextOutA(hdc, 10, 82, hint, (int)strlen(hint));
    }

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
    EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: PaintOverlay(hwnd); return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

DWORD WINAPI OverlayThread(LPVOID) {
    const char* CLASS_NAME = "TrainerOverlayWnd";
    HINSTANCE hInst = GetModuleHandleA(nullptr);

    WNDCLASSEXA wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = OverlayWndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassExA(&wc);

    // Window size to fit the text block
    int W = 330, H = 110;

    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        CLASS_NAME, "",
        WS_POPUP,
        12, 12, W, H,
        nullptr, nullptr, hInst, nullptr);

    if (!hwnd) return 1;
    g_overlayHwnd = hwnd;

    // Make window black = fully transparent (color key), rest visible
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd);

    g_overlayRunning = true;

    MSG msg;
    while (g_overlayRunning.load()) {
        // Invalidate every 100 ms so the HUD updates when saved-state changes
        if (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        } else {
            InvalidateRect(hwnd, nullptr, TRUE);
            Sleep(100);
        }
    }

    DestroyWindow(hwnd);
    UnregisterClassA(CLASS_NAME, hInst);
    return 0;
}

// Log helper
std::mutex g_logMutex;
void Log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::cout << "[Trainer] " << msg << std::endl;
    std::ofstream logFile("C:\\Users\\Dani\\Desktop\\Trainer\\trainer_log.txt", std::ios_base::app);
    if (logFile.is_open()) {
        logFile << "[Trainer] " << msg << "\n";
        logFile.close();
    }
}

void* GetNativePointer(void* managedObj) {
    if (!managedObj) return nullptr;
    return *(void**)((uintptr_t)managedObj + 0x10);
}

// Invoke helper
void* InvokeMethod(void* method, void* obj, void** params) {
    if (!method) return nullptr;
    void* exc = nullptr;
    void* res = il2cpp_runtime_invoke_fn(method, obj, params, &exc);
    if (exc) {
        Log("Exception caught during method invocation!");
        return nullptr;
    }
    return res;
}

// Find a method by name and parameter count and parameter type substring
void* FindMethod(void* klass, const char* methodName, int argsCount, const char* paramTypeSubstring = nullptr) {
    if (!klass) return nullptr;
    
    // First try the quick way
    void* method = il2cpp_class_get_method_from_name_fn(klass, methodName, argsCount);
    if (method && !paramTypeSubstring) {
        return method;
    }
    
    // If we need to match parameter types or the quick way failed, iterate methods
    void* iter = nullptr;
    while (void* m = il2cpp_class_get_methods_fn(klass, &iter)) {
        const char* name = il2cpp_method_get_name_fn(m);
        if (strcmp(name, methodName) == 0) {
            int pCount = il2cpp_method_get_param_count_fn(m);
            if (pCount == argsCount) {
                if (!paramTypeSubstring) {
                    return m;
                }
                // Check parameter type
                void* pType = il2cpp_method_get_param_fn(m, 0);
                const char* pName = il2cpp_type_get_name_fn(pType);
                if (strstr(pName, paramTypeSubstring) != nullptr) {
                    return m;
                }
            }
        }
    }
    return nullptr;
}

// Function to resolve all functions
bool ResolveIL2CPP() {
    HMODULE hGameAssembly = GetModuleHandleA("GameAssembly.dll");
    if (!hGameAssembly) {
        Log("ERROR: GameAssembly.dll not found in process memory.");
        return false;
    }

    // Resolve IL2CPP core exports
    il2cpp_domain_get_fn = (t_il2cpp_domain_get)GetProcAddress(hGameAssembly, "il2cpp_domain_get");
    il2cpp_thread_attach_fn = (t_il2cpp_thread_attach)GetProcAddress(hGameAssembly, "il2cpp_thread_attach");
    il2cpp_resolve_icall_fn = (t_il2cpp_resolve_icall)GetProcAddress(hGameAssembly, "il2cpp_resolve_icall");
    il2cpp_string_new_fn = (t_il2cpp_string_new)GetProcAddress(hGameAssembly, "il2cpp_string_new");

    // Resolve IL2CPP metadata exports
    il2cpp_domain_get_assemblies_fn = (t_il2cpp_domain_get_assemblies)GetProcAddress(hGameAssembly, "il2cpp_domain_get_assemblies");
    il2cpp_assembly_get_image_fn = (t_il2cpp_assembly_get_image)GetProcAddress(hGameAssembly, "il2cpp_assembly_get_image");
    il2cpp_image_get_name_fn = (t_il2cpp_image_get_name)GetProcAddress(hGameAssembly, "il2cpp_image_get_name");
    il2cpp_class_from_name_fn = (t_il2cpp_class_from_name)GetProcAddress(hGameAssembly, "il2cpp_class_from_name");
    il2cpp_class_get_methods_fn = (t_il2cpp_class_get_methods)GetProcAddress(hGameAssembly, "il2cpp_class_get_methods");
    il2cpp_method_get_name_fn = (t_il2cpp_method_get_name)GetProcAddress(hGameAssembly, "il2cpp_method_get_name");
    il2cpp_method_get_param_count_fn = (t_il2cpp_method_get_param_count)GetProcAddress(hGameAssembly, "il2cpp_method_get_param_count");
    il2cpp_method_get_param_fn = (t_il2cpp_method_get_param)GetProcAddress(hGameAssembly, "il2cpp_method_get_param");
    il2cpp_type_get_name_fn = (t_il2cpp_type_get_name)GetProcAddress(hGameAssembly, "il2cpp_type_get_name");
    il2cpp_class_get_method_from_name_fn = (t_il2cpp_class_get_method_from_name)GetProcAddress(hGameAssembly, "il2cpp_class_get_method_from_name");
    il2cpp_runtime_invoke_fn = (t_il2cpp_runtime_invoke)GetProcAddress(hGameAssembly, "il2cpp_runtime_invoke");
    il2cpp_class_get_type_fn = (t_il2cpp_class_get_type)GetProcAddress(hGameAssembly, "il2cpp_class_get_type");
    il2cpp_type_get_object_fn = (t_il2cpp_type_get_object)GetProcAddress(hGameAssembly, "il2cpp_type_get_object");

    if (!il2cpp_domain_get_fn || !il2cpp_thread_attach_fn || !il2cpp_resolve_icall_fn || !il2cpp_string_new_fn ||
        !il2cpp_domain_get_assemblies_fn || !il2cpp_assembly_get_image_fn || !il2cpp_image_get_name_fn ||
        !il2cpp_class_from_name_fn || !il2cpp_class_get_methods_fn || !il2cpp_method_get_name_fn ||
        !il2cpp_method_get_param_count_fn || !il2cpp_method_get_param_fn || !il2cpp_type_get_name_fn ||
        !il2cpp_class_get_method_from_name_fn || !il2cpp_runtime_invoke_fn || !il2cpp_class_get_type_fn ||
        !il2cpp_type_get_object_fn) {
        Log("ERROR: Failed to resolve core IL2CPP metadata APIs.");
        return false;
    }

    Log("Attached successfully to GameAssembly.dll core & metadata exports.");

    // Attach current thread to IL2CPP
    void* domain = il2cpp_domain_get_fn();
    il2cpp_thread_attach_fn(domain);
    Log("Thread attached to IL2CPP domain.");

    // 1. Resolve direct transform icalls (which we know are successful on the user's PC)
    Transform_get_position_fn = (t_Transform_get_position)il2cpp_resolve_icall_fn("UnityEngine.Transform::get_position_Injected(UnityEngine.Vector3&)");
    Transform_set_position_fn = (t_Transform_set_position)il2cpp_resolve_icall_fn("UnityEngine.Transform::set_position_Injected(UnityEngine.Vector3&)");
    Transform_get_rotation_fn = (t_Transform_get_rotation)il2cpp_resolve_icall_fn("UnityEngine.Transform::get_rotation_Injected(UnityEngine.Quaternion&)");
    Transform_set_rotation_fn = (t_Transform_set_rotation)il2cpp_resolve_icall_fn("UnityEngine.Transform::set_rotation_Injected(UnityEngine.Quaternion&)");

    if (!Transform_get_position_fn || !Transform_set_position_fn) {
        Log("ERROR: Direct Transform position icalls could not be resolved.");
        return false;
    }
    Log("Transform position & rotation icalls resolved successfully.");

    // 2. Discover assemblies & images in the domain
    size_t assembliesCount = 0;
    void** assemblies = il2cpp_domain_get_assemblies_fn(domain, &assembliesCount);
    if (!assemblies || assembliesCount == 0) {
        Log("ERROR: Failed to retrieve assemblies list from domain.");
        return false;
    }

    for (size_t i = 0; i < assembliesCount; ++i) {
        void* img = il2cpp_assembly_get_image_fn(assemblies[i]);
        if (!img) continue;
        
        const char* name = il2cpp_image_get_name_fn(img);
        if (strcmp(name, "UnityEngine.CoreModule") == 0 || strcmp(name, "UnityEngine.CoreModule.dll") == 0) {
            coreImage = img;
        } else if (strcmp(name, "UnityEngine.PhysicsModule") == 0 || strcmp(name, "UnityEngine.PhysicsModule.dll") == 0) {
            physicsImage = img;
        } else if (strcmp(name, "UnityEngine.Physics2DModule") == 0 || strcmp(name, "UnityEngine.Physics2DModule.dll") == 0) {
            physics2DImage = img;
        }
    }

    if (!coreImage) {
        Log("ERROR: UnityEngine.CoreModule image not found!");
        return false;
    }
    Log("Unity Core Module image resolved.");

    // 3. Resolve classes from images
    gameObjectClass = il2cpp_class_from_name_fn(coreImage, "UnityEngine", "GameObject");
    transformClass = il2cpp_class_from_name_fn(coreImage, "UnityEngine", "Transform");
    if (physicsImage) {
        rbClass = il2cpp_class_from_name_fn(physicsImage, "UnityEngine", "Rigidbody");
    }
    if (physics2DImage) {
        rb2DClass = il2cpp_class_from_name_fn(physics2DImage, "UnityEngine", "Rigidbody2D");
    }

    if (!gameObjectClass || !transformClass) {
        Log("ERROR: Failed to resolve core GameObject/Transform classes from metadata.");
        return false;
    }

    // 4. Resolve GameObject methods via metadata
    GameObject_Find_Method = FindMethod(gameObjectClass, "Find", 1);
    GameObject_FindWithTag_Method = FindMethod(gameObjectClass, "FindWithTag", 1);
    GameObject_get_transform_Method = FindMethod(gameObjectClass, "get_transform", 0);
    GameObject_GetComponent_Method = FindMethod(gameObjectClass, "GetComponent", 1, "Type");

    if (!GameObject_Find_Method || !GameObject_get_transform_Method || !GameObject_GetComponent_Method) {
        Log("ERROR: Failed to resolve critical GameObject/Component methods from metadata.");
        return false;
    }
    Log("All GameObject/Transform metadata methods resolved successfully.");

    // 5. Resolve Rigidbody / Rigidbody2D velocity methods from metadata
    if (rbClass) {
        Rigidbody_set_velocity_Method = FindMethod(rbClass, "set_velocity", 1);
        Rigidbody_set_angularVelocity_Method = FindMethod(rbClass, "set_angularVelocity", 1);
        Log("Rigidbody 3D methods: " + std::string(Rigidbody_set_velocity_Method ? "Velocity OK. " : "Velocity FAILED. ") +
                                     std::string(Rigidbody_set_angularVelocity_Method ? "Angular OK." : "Angular FAILED."));
    }

    if (rb2DClass) {
        Rigidbody2D_set_velocity_Method = FindMethod(rb2DClass, "set_velocity", 1);
        Rigidbody2D_set_angularVelocity_Method = FindMethod(rb2DClass, "set_angularVelocity", 1);
        Log("Rigidbody 2D methods: " + std::string(Rigidbody2D_set_velocity_Method ? "Velocity OK. " : "Velocity FAILED. ") +
                                     std::string(Rigidbody2D_set_angularVelocity_Method ? "Angular OK." : "Angular FAILED."));
    }

    Log("[+] SUCCESS: All engine interfaces initialized perfectly!");
    return true;
}

// Attempts to find the player GameObject by trying several typical names/tags
void* FindPlayer() {
    void* playerGo = nullptr;

    // 1. Try FindWithTag("Player")
    if (GameObject_FindWithTag_Method) {
        Log("Searching player via FindWithTag('Player')...");
        void* tagStr = il2cpp_string_new_fn("Player");
        void* params[] = { tagStr };
        playerGo = InvokeMethod(GameObject_FindWithTag_Method, nullptr, params);
        if (playerGo) {
            Log("Player found via FindWithTag('Player').");
            return playerGo;
        }
    }

    // 2. Try common GameObject names
    if (GameObject_Find_Method) {
        const char* commonNames[] = { "Flippy", "Player", "Phone", "PlayerPhone", "RetroPhone", "CellPhone", "Nokia", "BrickPhone" };
        for (const char* name : commonNames) {
            Log("Searching player via Find('" + std::string(name) + "')...");
            void* nameStr = il2cpp_string_new_fn(name);
            void* params[] = { nameStr };
            playerGo = InvokeMethod(GameObject_Find_Method, nullptr, params);
            if (playerGo) {
                Log("Player found via Find('" + std::string(name) + "').");
                return playerGo;
            }
        }
    }

    Log("Player not found in this frame.");
    return nullptr;
}

// Reset physics velocity of the player (supporting both 3D and 2D)
void ResetVelocity(void* playerGo) {
    if (!GameObject_GetComponent_Method) return;

    // --- Try 3D Rigidbody ---
    if (rbClass) {
        void* rbType = il2cpp_type_get_object_fn(il2cpp_class_get_type_fn(rbClass));
        void* params[] = { rbType };
        void* rb = InvokeMethod(GameObject_GetComponent_Method, playerGo, params);
        if (rb) {
            Vector3 zero3D = { 0.0f, 0.0f, 0.0f };
            void* velParams[] = { &zero3D };
            if (Rigidbody_set_velocity_Method) {
                InvokeMethod(Rigidbody_set_velocity_Method, rb, velParams);
            }
            if (Rigidbody_set_angularVelocity_Method) {
                InvokeMethod(Rigidbody_set_angularVelocity_Method, rb, velParams);
            }
            Log("Player 3D physical velocity reset to 0.");
            return;
        }
    }

    // --- Try 2D Rigidbody ---
    if (rb2DClass) {
        void* rb2DType = il2cpp_type_get_object_fn(il2cpp_class_get_type_fn(rb2DClass));
        void* params[] = { rb2DType };
        void* rb2d = InvokeMethod(GameObject_GetComponent_Method, playerGo, params);
        if (rb2d) {
            Vector2 zero2D = { 0.0f, 0.0f };
            void* velParams[] = { &zero2D };
            if (Rigidbody2D_set_velocity_Method) {
                InvokeMethod(Rigidbody2D_set_velocity_Method, rb2d, velParams);
            }
            
            float zeroFloat = 0.0f;
            void* angParams[] = { &zeroFloat };
            if (Rigidbody2D_set_angularVelocity_Method) {
                InvokeMethod(Rigidbody2D_set_angularVelocity_Method, rb2d, angParams);
            }
            Log("Player 2D physical velocity reset to 0.");
            return;
        }
    }
}

DWORD WINAPI TrainerThread(LPVOID lpParam) {
    // Truncate log file
    {
        std::ofstream logFile("C:\\Users\\Dani\\Desktop\\Trainer\\trainer_log.txt", std::ios_base::trunc);
        if (logFile.is_open()) {
            logFile << "=== Trainer Log Start ===\n";
            logFile.close();
        }
    }
    // Spawn debug console
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);

    std::cout << "=========================================" << std::endl;
    std::cout << "   Flipping is Hard speedrun Practice   " << std::endl;
    std::cout << "=========================================" << std::endl;
    Log("Initializing practice tool...");

    // Wait for the game to fully load and GameAssembly to be available
    while (GetModuleHandleA("GameAssembly.dll") == nullptr) {
        Sleep(500);
    }

    // Resolve IL2CPP and Unity APIs
    if (!ResolveIL2CPP()) {
        Log("[!] ERROR: Initialization failed. Press Enter to exit.");
        std::cin.get();
        FreeConsole();
        FreeLibraryAndExitThread((HMODULE)lpParam, 0);
        return 0;
    }

    Log("[+] Mod active! Controls:");
    Log("    [Shift + R] -> Save position");
    Log("    [R]         -> Teleport to saved position");
    Log("=========================================");

    // Launch overlay HUD thread
    CreateThread(nullptr, 0, OverlayThread, nullptr, 0, nullptr);

    bool rPressedLast = false;

    while (true) {
        // Sleep to avoid consuming 100% CPU
        Sleep(16); // ~60 Hz

        // Check keys
        bool isShiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool isRDown = (GetAsyncKeyState('R') & 0x8000) != 0;

        // Ensure we attach the IL2CPP runtime on each check frame to avoid thread collisions
        void* domain = il2cpp_domain_get_fn();
        il2cpp_thread_attach_fn(domain);

        // Edge detection for R key press (trigger once on down event)
        if (isRDown && !rPressedLast) {
            if (isShiftDown) {
                // --- SAVE POSITION ---
                Log("Shift + R detected. Saving position...");
                void* playerGo = FindPlayer();
                if (playerGo) {
                    Log("Getting transform component...");
                    void* transform = InvokeMethod(GameObject_get_transform_Method, playerGo, nullptr);
                    if (transform) {
                        void* nativeTransform = GetNativePointer(transform);
                        if (nativeTransform) {
                            Log("Calling Transform_get_position_fn with native pointer...");
                            Transform_get_position_fn(nativeTransform, &g_savedPos);
                            
                            Log("Calling Transform_get_rotation_fn with native pointer...");
                            if (Transform_get_rotation_fn) {
                                Transform_get_rotation_fn(nativeTransform, &g_savedRot);
                            }
                            g_hasSavedPos = true;
                            g_hasSavedPosOverlay = true;
                            
                            char buf[128];
                            sprintf_s(buf, "Position saved! (X: %.2f, Y: %.2f, Z: %.2f)", g_savedPos.x, g_savedPos.y, g_savedPos.z);
                            Log(buf);
                        } else {
                            Log("WARNING: Native transform pointer is NULL!");
                        }
                    } else {
                        Log("WARNING: Found player GameObject, but it has no Transform!");
                    }
                } else {
                    Log("WARNING: Player GameObject not found! Cannot save position.");
                    Log("Tip: Try to load into the gameplay first, then press Shift+R.");
                }
            } else {
                // --- RESTORE POSITION ---
                Log("R detected. Restoring position...");
                if (g_hasSavedPos) {
                    void* playerGo = FindPlayer();
                    if (playerGo) {
                        Log("Getting transform component...");
                        void* transform = InvokeMethod(GameObject_get_transform_Method, playerGo, nullptr);
                        if (transform) {
                            void* nativeTransform = GetNativePointer(transform);
                            if (nativeTransform) {
                                Log("Calling Transform_set_position_fn with native pointer...");
                                // Teleport position and rotation
                                Transform_set_position_fn(nativeTransform, &g_savedPos);
                                
                                Log("Calling Transform_set_rotation_fn with native pointer...");
                                if (Transform_set_rotation_fn) {
                                    Transform_set_rotation_fn(nativeTransform, &g_savedRot);
                                }

                                Log("Resetting physics velocity...");
                                // Reset physical velocity
                                ResetVelocity(playerGo);

                                char buf[128];
                                sprintf_s(buf, "Teleported to saved position! (X: %.2f, Y: %.2f, Z: %.2f)", g_savedPos.x, g_savedPos.y, g_savedPos.z);
                                Log(buf);
                                

                            } else {
                                Log("WARNING: Native transform pointer is NULL!");
                            }
                        } else {
                            Log("WARNING: Found player GameObject, but it has no Transform!");
                        }
                    } else {
                        Log("WARNING: Player GameObject not found! Cannot teleport.");
                    }
                } else {
                    Log("WARNING: No position saved yet! Press Shift + R to save your position first.");
                }
            }
        }

        rPressedLast = isRDown;

        // Quick way to unload trainer if we press END key
        if (GetAsyncKeyState(VK_END) & 0x8000) {
            Log("Unloading trainer...");
            break;
        }
    }

    // Cleanup
    FreeConsole();
    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)TrainerThread, hModule, 0, nullptr);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
