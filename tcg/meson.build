if not have_tcg
   subdir_done()
endif

tcg_ss = ss.source_set()

tcg_ss.add(files(
  'optimize.c',
  'region.c',
  'tcg.c',
  'tcg-common.c',
  'tcg-op.c',
  'tcg-op-ldst.c',
  'tcg-op-gvec.c',
  'tcg-op-vec.c',
))

tcg_interpreter = get_option('tcg_interpreter')
if tcg_interpreter != 'false'
  libffi = dependency('libffi', version: '>=3.0', required: true,
                      method: 'pkg-config')
  tcg_ss.add(libffi)

  if tcg_interpreter == 'both'
    # In dual mode, tci.c must be compiled with the TCI include path
    # as primary, since it needs the TCI-specific TCGReg, opcodes, etc.
    # The native arch include path (tcg_native_inc from top-level meson.build)
    # is added only to tcg_ss so native TCG core files find the right headers.
    tci_inc = include_directories('tci')
    tci_lib = static_library('tci',
      files('tci.c'),
      include_directories: tci_inc,
      dependencies: libffi)
    tci_dep = declare_dependency(link_with: tci_lib)
    native_inc_dep = declare_dependency(include_directories: tcg_native_inc)
    tcg_ss.add(when: tci_dep, if_true: [])
    tcg_ss.add(native_inc_dep)
  else
    tcg_ss.add(files('tci.c'))
  endif
endif

tcg_ss.add(when: libdw, if_true: files('debuginfo.c'))
if host_os == 'linux'
  tcg_ss.add(files('perf.c'))
endif

user_ss.add_all(tcg_ss)
system_ss.add_all(tcg_ss)
