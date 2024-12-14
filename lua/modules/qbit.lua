-- Load qbit config
local module = {
    cookie = nil,
    config = nil,
    config_path = system_paths.configs .. "/qbit.json"
}

function module.get_config(reload)
    if module.config ~= nil and not reload then
        return module.config
    end

    local file = io.open(module.config_path, "r")

    if file then
        local text = file:read("*a")
        file:close()

        module.config = json.decode(text)
        return module.config
    else
        file = io.open(module.config_path, "w")

        module.config = {
            login = {
                username = "admin",
                password = "admin",
            },
            api_url = "http://localhost:8080/api/v2",
            download_dir = system_paths.home .. "/Anime/Series"
        }

        print("Make sure to edit \"" .. module.config_path .. "\" with your username and password.")

        file:write(json.encode(module.config))
        file:close()
        return module.config
    end
end

function module.authenticate()
    if module.cookie then
        return module.cookie
    end

    local config = module.get_config()

    local response = requests.post({
        url = config.api_url .. "/auth/login",
        body = "username=" .. config.login.username .. "&password=" .. config.login.password,
        headers = {
            ["Content-Type"] = "application/x-www-form-urlencoded"
        }
    })

    if response.status_code ~= 200 then
        print("Failed to authenticate: " .. response.data)
        return
    end

    module.cookie = response.headers["set-cookie"]
end

function module.request(data)
    local config = module.get_config()

    if module.cookie == nil then
        module.authenticate()
    end

    data.url = config.api_url .. "/" .. data.endpoint
    data.endpoint = nil
    if not data.headers then
        data.headers = {}
    end
    data.headers.Cookie = module.cookie

    return requests.make(data)
end

function module.get(endpoint)
    return module.request({
        method = "GET",
        endpoint = endpoint,
    })
end

function module.post(data)
    data.method = "POST"

    return module.request(data)
end

function module.download_magnet(magnet)
    local config = module.get_config()

    return module.post({
        endpoint = "torrents/add",
        body = "urls=" .. magnet .. "&savepath=" .. config.download_dir,
        headers = {
            ["Content-Type"] = "application/x-www-form-urlencoded"
        }
    })
end

return module
