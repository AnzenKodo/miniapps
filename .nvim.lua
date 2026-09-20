-- ak: Build Initialization
-- ============================================================================

local build_dir = "build"
local build_command = ""
local cc_command = ""
local program_name = "play"
if (vim.fn.has('win32') == 1 or vim.fn.has('win64') == 1) then
    build_command = "setup_x64.bat && cl.exe build.c -nologo -Z7 -Fo:build\\ -Fe:"..build_dir.."\\a.exe"
    cc_command = "a.exe "
else
    build_command = "clang -ggdb build.c"
    cc_command = "./a.out "
end

-- ak: Build user commands
-- ============================================================================

-- NOTE(ak): run with `:make` below command

vim.opt.makeprg = cc_command .. "build-dry " .. program_name
vim.api.nvim_create_user_command("BuildRun",  function()
    vim.opt.makeprg = cc_command .. "build-run " .. program_name
    vim.cmd('make')
    vim.opt.makeprg = cc_command .. "build-dry " .. program_name
end, { desc = "Bootstrap build system"})
vim.api.nvim_create_user_command("BuildBuild",  function()
    vim.opt.makeprg = build_command
    vim.cmd('make')
    vim.opt.makeprg = cc_command .. "build-dry " .. program_name
end, { desc = "Bootstrap build system"})
vim.api.nvim_create_user_command("BuildMeta",  function()
    vim.opt.makeprg = cc_command .. "gen-meta"
    vim.cmd('make')
    vim.opt.makeprg = cc_command .. "build-dry " .. program_name
    local success = vim.v.shell_error == 0
    if (success) then
        vim.cmd('make')
    end
end, { desc = "Bootstrap build system"})

vim.keymap.set("n", "<F5>", "<CMD>BuildRun<CR>", { desc = "Build Run" })
