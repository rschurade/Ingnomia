# For a list of common variables see https://github.com/microsoft/vcpkg/blob/master/docs/maintainers/vcpkg_common_definitions.md

# Download source packages
# (bgfx requires bx and bimg source for building)

vcpkg_from_github(OUT_SOURCE_PATH BX_SOURCE_PATH
    REPO "bkaradzic/bx"
    HEAD_REF master
    REF aa5090bbd8c39e84d483f4850a7d9ca7ff9241ed
    SHA512 7b95ff924ee22bbda67783ad06e37869c24e6d1cc67bcac37deb0998398d26c83af737c8b8e4c6cde07d785d80f70157e197549a6b2041cde46793fe8ca30efc
)

vcpkg_from_github(OUT_SOURCE_PATH BIMG_SOURCE_PATH
    REPO "bkaradzic/bimg"
    HEAD_REF master
    REF 663f724186e26caf46494e389ed82409106205fb
    SHA512 6fcd6d06d0d55a8f573a882bb57b729ee22101314a201bd8f440bdcadbfeb0dca6834302ef7dcfdee785d87b5456d24d2cb630d1d39836b4cd7652a6dc92d5a8
)

vcpkg_from_github(OUT_SOURCE_PATH BGFX_SOURCE_PATH
    REPO "bkaradzic/bgfx"
    HEAD_REF master
    REF db44d5675f4e02a9f3178fe54ac6d5a9c216a573
    SHA512 207b7fcd10a0d1205984a2ef2360ff410d4aa37b7f87e4bd0562cbedaecd87ef1666746035cb99ad8614cea8abd57e467e0a1ffda9f5b7266351e58a9e2fff5d
)

vcpkg_from_github(OUT_SOURCE_PATH SOURCE_PATH
    REPO "bkaradzic/bgfx.cmake"
    HEAD_REF master
    REF 12b75cc0ad0078a700d4854db16e119ef4706347
    SHA512 35e0d84cd427f34036d0ab6404bdd90147f36bb0483cbc575107a6e2ed77a150bf943858db9bd8a091a1a6a02d3821d4d93181fa5997507686d34a8ecc22a318
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBX_DIR:PATH="${BX_SOURCE_PATH}"
        -DBIMG_DIR:PATH="${BIMG_SOURCE_PATH}"
        -DBGFX_DIR:PATH="${BGFX_SOURCE_PATH}"
        -DBGFX_BUILD_EXAMPLES:BOOL=OFF
        -DBGFX_OPENGL_VERSION:STRING=43
    OPTIONS_DEBUG
        -DBGFX_BUILD_TOOLS:BOOL=OFF
)

vcpkg_cmake_install()

# Move license
vcpkg_install_copyright(FILE_LIST "${CURRENT_PACKAGES_DIR}/share/licences/bgfx/LICENSE")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/licences")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share/licences")

# Remove debug includes
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/bgfx")
vcpkg_copy_tools(
    TOOL_NAMES
        geometryc
        geometryv
        shaderc
        texturec
        texturev
    AUTO_CLEAN
)
vcpkg_copy_pdbs()
