if has('win32') || has('win64')
  let &makeprg=".\\scripts\\make_win32.cmd $*"
  let g:nmakeprg=".\\scripts\\make_win32.cmd"
  let g:gtest#gtest_command = "cd build2 && .\\unittests"
else 
  let &makeprg="./scripts/make_unix.sh $*"
  let g:nmakeprg="./scripts/make_unix.sh"
  let g:gtest#gtest_command = "cd build && ./unittests"
endif
set path+=.,include/**,src/**,tests/**,3rd_party/json/**,3rd_party/jsonrpcpp-1.1.1/lib/**,3rd_party/ttmath-0.9.3/ttmath/**

lua << EOF
if vim.env.NV_DARCULA_ENABLE_DAP then
  local dap = require("dap")
  local g = require("nvim-goodies")
  local gos = require("nvim-goodies.os")
  local gpath = require("nvim-goodies.path")

  local externalConsole = gos.is_windows
  local type = g.iff(gos.is_windows, "cppvsdbg", "cppdbg")
  local cwd = gpath.create_path(".", g.iff(gos.is_windows, "build2//debug", "build"))
  local mount_args = {"-f", g.iff(gos.is_windows, "T:", "~/mnt") }
  local test_args = {"--gtest_filter=encrypt*"}
  dap.configurations.cpp = {
    {
      name = "Mount",
      type = type,
      request = "launch",
      program = function()
        return gpath.create_path(cwd, "repertory")
      end,
      args=mount_args,
      cwd = cwd,
      stopAtEntry = true,
      externalConsole=externalConsole,
    },
    {
      name = "Test",
      type = type,
      request = "launch",
      program = function()
        return gpath.create_path(cwd, "unittests")
      end,
      args=test_args,
      cwd = cwd,
      stopAtEntry = true,
      externalConsole=externalConsole,
    }
  }
else
  vim.api.nvim_create_autocmd(
    "VimEnter",
    {
      group = vim.api.nvim_create_augroup("nvimrc_cfg", {clear = true}),
      pattern = "*",
      callback = function()
        vim.api.nvim_command("VimspectorLoadSession .default_session")
      end
    }
  )
end
EOF
