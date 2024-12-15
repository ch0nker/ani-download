local qbit = require("qbit")
local config = require("config"):new("nyaa", {
    page_size = 10
})

ui.init()

local current_page = 1

local function get_torrent(torrents)
    local page = 1
    local start_index
    local page_size = config.page_size
    local result

    local total_torrents = #torrents
    local max_pages = math.ceil(total_torrents / page_size)

    local window = ui.create({
        width = ui.cols,
        height = page_size + 9
    })

    local x, y = 1, 1
    local space = 2

    ui.init_pair(1, ui.color.black, ui.color.white)
    ui.init_pair(2, ui.color.white, ui.color.black)

    window:set_color(2)

    local function output_page()
        window:clear()

        x = 1
        y = 1

        page = math.max(1, math.min(page, max_pages))
        start_index = (page - 1) * page_size + 1
        local end_index = math.min(start_index + page_size - 1, total_torrents)
        local page_title = "Page: " .. page .. "/" .. max_pages

        window:print((window.width - #page_title) / 2, 1, page_title)
        local row = 1
        for i = start_index, end_index do
            local torrent = torrents[i]
            if row == y then
                window:set_color(1)
                window:print(1, row + space, string.format("%s (%s)", torrent.full_title:gsub("[^\x00-\x7F]", ""):sub(x, x + (window.width - (#torrent.size + 6))), torrent.size))
                window:clear_color()
            else
                window:print(1, row + space, string.format("%s (%s)", torrent.full_title:gsub("[^\x00-\x7F]", ""):sub(1, window.width - (#torrent.size + 5)), torrent.size))
            end
            row = row + 1
        end

        local enter_text = "enter: Download Torrent"
        local next_text = "n: Next Page"
        local previous_text = "p: Previous Page"
        local quit_text = "q: Exit"

        local half_start = (window.width - #enter_text) / 2

        window:print(half_start, window.height - 5, enter_text)
        window:print(half_start, window.height - 4, next_text)
        window:print(half_start, window.height - 3, previous_text)
        window:print(half_start, window.height - 2, quit_text)
    end

    output_page()

    ui.on_key(function (key)
        if key == "n" then
            page = page + 1
            output_page()
            return true
        elseif key == "\n" then
            result = torrents[start_index + y - 1]
            return false
        elseif key == "p" then
            page = page - 1
            output_page()
            return true
        end

        local torrent = torrents[start_index + y - 1]

        window:set_color(2)
        window:print(1, y + space, string.format("%s (%s)", torrent.full_title:gsub("[^\x00-\x7F]", ""):sub(1, window.width - (#torrent.size + 5)), torrent.size))

        if key == "UP" then
            y = math.max(1, y - 1)
            x = 1
        elseif key == "DOWN" then
            y = math.min(y + 1, page_size)
            x = 1
        elseif key == "LEFT" then
            x = math.max(1, x - 1)
        end

        torrent = torrents[start_index + y - 1]

        if key == "RIGHT" then
            local text_length = #torrent.full_title:gsub("[^\x00-\x7F]", "")
            if x + (window.width - (#torrent.size + 6)) < text_length then
                x = math.min(x + 1, text_length - (window.width - (#torrent.size + 6)))
            end
        end

        window:set_color(1)

        window:print(1, y + space, string.format("%s (%s)", torrent.full_title:gsub("[^\x00-\x7F]", ""):sub(x, x + (window.width - (#torrent.size + 6))), torrent.size))

        window:clear_color()

        return key ~= "q"
    end)

    window:destroy()
    return result
end

local function download(title)
    local response = requests.get({
        url = "https://nyaa.land/?f=0&c=1_2&p=" .. tostring(current_page) .. "&q=" .. (requests.url_encode(title) or "")
    })

    local parser = html.parse(response.data)
    local torrent_elms = parser:xpath("//tbody")[1]
    local torrents = {}

    for _, elm in next, torrent_elms.children do
        local title_elm = elm:xpath(".//td[@colspan=\"2\"]/a[not(@class=\"comments\")]")[1]
        local comments_elm = elm:xpath(".//td[@colspan=\"2\"]/a[@class=\"comments\"]")[1]
        local comments = 0

        if comments_elm then
            comments = tonumber(comments_elm.text:match("%s*(%d+)"))
        end

        local full_title = title_elm.attributes.title
        local download_elm = elm.children[3]
        local time_elm = elm.children[5]

        local torrent = {
            full_title = full_title,
            link = "https://nyaa.land" .. title_elm.attributes.href,
            uploader = full_title:match("^%[(%w+)%]"),
            status = elm.attributes.class,
            size = elm.children[4].text,
            magnet = download_elm.children[2].attributes.href,
            torrent = download_elm.children[1].attributes.href,
            time = {
                timestamp = tonumber(time_elm.attributes["data-timestamp"]),
                title = time_elm.attributes.title,
                date = time_elm.text
            },
            comments = comments,
            seeders = tonumber(elm.children[6].text),
            leechers = tonumber(elm.children[7].text),
            total_downloads = tonumber(elm.children[7].text)
        }

        table.insert(torrents, torrent)
    end

    local torrent
    if #torrents > 1 then
        torrent = get_torrent(torrents)
    else
        torrent = torrents[1]
    end

    if not torrent then return end

    os.execute("qbittorrent")
    qbit.download_magnet(torrent.magnet)
end

return {
    name = "nyaa",
    description = "Scrapes from https://nyaa.land then uses qbittorent to download.",
    version = "0.0.1",
    type = "torrent",
    download = download
}
