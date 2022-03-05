if has('win32') || has('win64')
  let &makeprg=".\\scripts\\make_win32.cmd $*"
  let g:gtest#gtest_command = "cd build2 && .\\unittests"
else 
  let &makeprg="./scripts/make_unix.sh $*"
  let g:gtest#gtest_command = "cd build && ./unittests"
endif
set path+=.,include/**,src/**,tests/**,3rd_party/json/**,3rd_party/jsonrpcpp-1.1.1/lib/**,3rd_party/ttmath-0.9.3/ttmath/**
autocmd! VimEnter * :VimspectorLoadSession .default_session
