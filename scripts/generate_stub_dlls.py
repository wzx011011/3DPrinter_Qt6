"""
Generate OCCT stub DLLs to allow FramelessDialogDemo.exe to load.
The exe imports OCCT functions that are never called in the QML GUI.
Stub DLLs export all required functions as no-op stubs.
"""
import subprocess
import sys
import os

OCCT_LIB_DIR = r"E:/ai/3D-Printer/deps/build/OrcaSlicer_dep/usr/local/lib/occt"
BUILD_DIR = r"E:/ai/3DPrinter_Qt6/build"
STUB_SRC_DIR = os.path.join(BUILD_DIR, "_stub_gen")

# OCCT DLLs that the exe imports directly
DIRECT_DLLS = [
    "TKXDESTEP", "TKXCAF", "TKLCAF", "TKMesh", "TKPrim",
    "TKTopAlgo", "TKBRep", "TKMath", "TKernel",
    "cr_tpms_library"
]

# cr_tpms is from a different directory
CR_TPMS_LIB = os.path.join(
    r"E:/ai/3D-Printer/deps/build/OrcaSlicer_dep/usr/local/lib",
    "cr_tpms_library.lib"
)

def get_exports(lib_path):
    """Get exported function names from a .lib file."""
    result = subprocess.run(
        ["dumpbin", "/exports", lib_path],
        capture_output=True, text=True, timeout=30
    )
    exports = []
    in_exports = False
    for line in result.stdout.split('\n'):
        line = line.strip()
        if line.startswith("ordinal"):
            in_exports = True
            continue
        if in_exports:
            if not line:
                continue
            # Format: [ordinal] [hint] [name]
            parts = line.split()
            if len(parts) >= 3:
                name = parts[2]
                # Skip forwarders
                if name and not name.startswith('[forwarded'):
                    exports.append(name)
            elif line.startswith("Summary") or line.startswith("Section"):
                break
    return exports


def get_occt_dll_deps(dll_name):
    """Get dependencies of an OCCT DLL."""
    dll_path = os.path.join(OCCT_LIB_DIR, dll_name + ".lib")
    if not os.path.exists(dll_path):
        return []
    result = subprocess.run(
        ["dumpbin", "/dependents", dll_path],
        capture_output=True, text=True, timeout=30
    )
    deps = []
    for line in result.stdout.split('\n'):
        line = line.strip()
        if line.lower().endswith('.dll'):
            dep = line.replace('.dll', '').replace('.DLL', '')
            if dep.startswith('TK'):
                deps.append(dep)
    return deps


def collect_all_dlls():
    """Collect all needed OCCT DLLs (transitive dependencies)."""
    needed = set()
    queue = list(DIRECT_DLLS)
    while queue:
        dll = queue.pop()
        if dll in needed:
            continue
        needed.add(dll)
        # Get transitive OCCT deps
        if dll.startswith('TK'):
            lib_path = os.path.join(OCCT_LIB_DIR, dll + ".lib")
            if os.path.exists(lib_path):
                for dep in get_occt_dll_deps(dll):
                    if dep not in needed:
                        queue.append(dep)
    return sorted(needed)


def generate_stub_dll(dll_name, exports):
    """Generate a stub DLL source files for a given DLL."""
    dll_dir = os.path.join(STUB_SRC_DIR, dll_name)
    os.makedirs(dll_dir, exist_ok=True)

    # Create .def file with all exports
    def_path = os.path.join(dll_dir, dll_name + ".def")
    with open(def_path, "w") as f:
        f.write(f"LIBRARY {dll_name}\n")
        f.write("EXPORTS\n")
        for exp in exports:
            f.write(f"    {exp} = _stub_fn\n")

    # Create stub implementation
    cpp_path = os.path.join(dll_dir, "stub.cpp")
    with open(cpp_path, "w") as f:
        f.write('// Stub DLL - all exports point to this single no-op function.\n')
        f.write('extern "C" {\n')
        f.write('// Generic stub: returns 0 (nullptr) in RAX.\n')
        f.write('// x64 calling convention: RCX, RDX, R8, R9 = args; RAX = return.\n')
        f.write('__declspec(dllexport) void* __cdecl _stub_fn() { return (void*)0; }\n')
        f.write('}\n')

    return def_path, cpp_path


def main():
    os.makedirs(STUB_SRC_DIR, exist_ok=True)

    dlls = collect_all_dlls()
    print(f"Need {len(dlls)} DLLs: {dlls}")

    for dll_name in dlls:
        if dll_name == "cr_tpms_library":
            lib_path = CR_TPMS_LIB
        else:
            lib_path = os.path.join(OCCT_LIB_DIR, dll_name + ".lib")

        if not os.path.exists(lib_path):
            print(f"  WARNING: lib not found: {lib_path}")
            continue

        exports = get_exports(lib_path)
        print(f"  {dll_name}: {len(exports)} exports")
        def_path, cpp_path = generate_stub_dll(dll_name, exports)

    print(f"\nGenerated stub sources in: {STUB_SRC_DIR}")
    print("Now build each DLL:")
    print(f"  cd {STUB_SRC_DIR}")
    print(f"  cl /LD /MD stub.cpp {dll_name}.def /Fe:{dll_name}.dll /link /DEF:{dll_name}.def")


if __name__ == "__main__":
    main()
