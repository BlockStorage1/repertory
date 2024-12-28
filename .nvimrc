if has('win32') || has('win64')
  let &makeprg=".\\scripts\\make_win32.cmd"
  let g:nmakeprg=".\\scripts\\make_win32.cmd"
  " let g:gtest#gtest_command = "cd build2 && .\\unittests"
else 
  let &makeprg="./scripts/make_unix.sh"
  let g:nmakeprg="./scripts/make_unix.sh"
  " let g:gtest#gtest_command = "cd build && ./unittests"
endif
set path+=.,repertory/**

lua << EOF
if vim.env.NV_DARCULA_ENABLE_DAP then
  local dap = require("dap")
  local g = require("nvim-goodies")
  local gos = require("nvim-goodies.os")
  local gpath = require("nvim-goodies.path")

  local externalConsole = gos.is_windows
  local type =   "cppdbg"
  local cwd = gpath.create_path("./dist/debug/shared/linux/x86_64/repertory/")
  dap.configurations.cpp = {
    {
      name = "Mount",
      type = type,
      request = "launch",
      program = function()
        return gpath.create_path(cwd, "bin/repertory")
      end,
      cwd = cwd,
      stopAtEntry = true,
      externalConsole=externalConsole,
    },
    {
      name = "Test",
      type = type,
      request = "launch",
      program = function()
        return gpath.create_path(cwd, "bin/repertory_test")
      end,
      args={"--gtest_filter=utils_db*"},
      cwd = cwd,
      stopAtEntry = true,
      externalConsole=externalConsole,
    }
  }
end
EOF
