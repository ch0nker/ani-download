local config = require("config"):new("qbit", {
    login = {
        username = "admin",
        password = "admin",
    },
    api_url = "http://localhost:8080/api/v2",
    download_dir = system_paths.home .. "/Anime/Series"
})
local module = {
    cookie = nil,
}

function module.authenticate()
    if module.cookie then
        return module.cookie
    end

    local login = config.login

    local response = requests.post({
        url = config.api_url .. "/auth/login",
        body = "username=" .. login.username .. "&password=" .. login.password,
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
    return module.post({
        endpoint = "torrents/add",
        body = "urls=" .. magnet .. "&savepath=" .. config.download_dir,
        headers = {
            ["Content-Type"] = "application/x-www-form-urlencoded"
        }
    })
end

return module
