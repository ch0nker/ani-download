local module = {
    configs = {}
}

function module:new(name, default_config)
    local config = setmetatable({
        __default = default_config or {},
        __config = {},
        __config_path = string.format("%s/%s.json", system_paths.configs, name)
    }, {
        __index = function(this, index)
            local val = rawget(this, index)
            if type(val) == "function" then
                return val
            end

            return rawget(this, "__config")[index]
        end,
        __newindex = function(this, index, value)
            local config = rawget(this, "__config")

            config[index] = value

            local file = io.open(rawget(this, "__config_path"), "w")
            assert(file, "Failed to open config.")
            file:write(json.decode(config))
            file:close()
        end
    })

    function config.update_config(tbl, depth)
        local default_cfg = rawget(config, "__default")
        depth = depth or default_cfg

        for key, value in next, depth do
            if not tbl[key] then
                tbl[key] = value
            end

            if type(value) == "table" then
                tbl[key] = config.update_config(tbl[key], value)
            end
        end

        return tbl
    end

    function config.set_default(cfg)
        rawset(config, "__default", cfg)
    end

    local config_path = rawget(config, "__config_path")

    local file = io.open(config_path, "r")
    if file then
        local text = file:read("*a")
        file:close()

        rawset(config, "__config", config.update_config(json.decode(text)))
    else
        file = io.open(config_path, "w")
        assert(file, "Failed to open " .. config_path)

        local cfg = rawget(config, "__default")
        file:write(json.encode(cfg))
        file:close()
        rawset(config, "__config", cfg)
    end

    module.configs[name] = config

    return config
end

return module
