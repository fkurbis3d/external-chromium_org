# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Intended to be included by chrome_syzygy.gyp. A variable 'dll_name' should
# be set to the base name of the DLL. This is used to generate the build steps
# for both chrome.dll and chrome_child.dll when in multiple dll mode.
{
  # Reorder or instrument the initial chrome DLL executable, placing the
  # optimized output and corresponding PDB file into the "syzygy"
  # subdirectory.
  # This target won't build in fastbuild, since there are no PDBs.
  'dependencies': [
    '<(DEPTH)/chrome/chrome.gyp:<(dll_name)_dll',
  ],
  'variables': {
    'dest_dir': '<(PRODUCT_DIR)/syzygy',
  },
  'conditions': [
    ['asan!=1', {
      # Reorder chrome DLL executable.
      # If there's a matching chrome.dll-ordering.json file present in
      # the output directory, chrome.dll will be ordered according to
      # that, otherwise it will be randomized.
      'actions': [
        {
          'action_name': 'Reorder Chrome with Syzygy',
          'msvs_cygwin_shell': 0,
          'inputs': [
            '<(PRODUCT_DIR)/<(dll_name).dll',
            '<(PRODUCT_DIR)/<(dll_name).dll.pdb',
          ],
          'outputs': [
            '<(dest_dir)/<(dll_name).dll',
            '<(dest_dir)/<(dll_name).dll.pdb',
          ],
          'action': [
            'python',
            '<(DEPTH)/chrome/tools/build/win/syzygy_reorder.py',
            '--input_executable', '<(PRODUCT_DIR)/<(dll_name).dll',
            '--input_symbol', '<(PRODUCT_DIR)/<(dll_name).dll.pdb',
            '--destination_dir', '<(dest_dir)',
          ],
        },
      ],
    }, {
      # Instrument chrome DLL executable with SyzyAsan.
      'actions': [
        {
          'action_name': 'Instrument Chrome with SyzyAsan',
          'msvs_cygwin_shell': 0,
          'inputs': [
            '<(PRODUCT_DIR)/<(dll_name).dll',
            '<(PRODUCT_DIR)/<(dll_name).dll.pdb',
            '<(DEPTH)/chrome/tools/build/win/win-syzyasan-filter.txt',
          ],
          'outputs': [
            '<(dest_dir)/<(dll_name).dll',
            '<(dest_dir)/<(dll_name).dll.pdb',
            '<(dest_dir)/asan_rtl.dll',
            '<(dest_dir)/asan_rtl.dll.pdb',
            '<(dest_dir)/win-syzyasan-filter.txt.json',
          ],
          'action': [
            'python',
            '<(DEPTH)/chrome/tools/build/win/syzygy_instrument.py',
            '--mode', 'asan',
            '--input_executable', '<(PRODUCT_DIR)/<(dll_name).dll',
            '--input_symbol', '<(PRODUCT_DIR)/<(dll_name).dll.pdb',
            '--filter',
            '<(DEPTH)/chrome/tools/build/win/win-syzyasan-filter.txt',
            '--destination_dir', '<(dest_dir)',
          ],
        },
      ],
    }],
  ],
}
