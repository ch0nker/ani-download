local module = {
    loaded_config = nil,
    default_config = {}
}

function module.update_config(tbl, depth)
    for key, value in next, depth or module.default_config do
        if not tbl[key] then
            tbl[key] = value
        end

        if type(value) == "table" then
            tbl[key] = module.update_config(tbl[key], value)
        end
    end

    return tbl
end

function module.get_config()
    if module.loaded_config ~= nil then return module.loaded_config end

    local file = io.open(module.config_path)
    if file then
        local text = file:read("*a")
        file:close()

        module.loaded_config = module.update_config(json.decode(text))

        return module.loaded_config
    end

    file = io.open(module.config_path, "w")
    assert(file, "Failed to open " .. module.config_path)

    module.loaded_config = module.default_config
    file:write(json.encode(module.loaded_config))
    file:close()

    return module.loaded_config
end

function module.get(key)
    local traceback = debug.traceback()
    module.caller_path = traceback:match(".-/config.lua:%d+: in function 'config%.get'%s*\n%s*(.-):%d+:")
    module.caller_script = module.caller_path and module.caller_path:match("([^/]+)%.lua$")
    module.config_path = string.format("%s/%s.json", system_paths.configs, module.caller_script)

    local config = module.get_config()

    return config and config[key]
end

function module.set(key, value)
    local traceback = debug.traceback()
    module.caller_path = traceback:match(".-/config.lua:%d+: in function 'config%.set'%s*\n%s*(.-):%d+:")
    module.caller_script = module.caller_path and module.caller_path:match("([^/]+)%.lua$")
    module.config_path = string.format("%s/%s.json", system_paths.configs, module.caller_script)

    local config = module.get_config()

    config[key] = value

    local file = io.open(module.config_path, "w")
    assert(file, "Failed to open " .. module.config_path)
    file:write(json.encode(config))
    file:close()
end

return module
