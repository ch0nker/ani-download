# Cores
You can create custom cores by writing a Lua script and saving it in the following directory:
```plaintext
~/.config/ani-downloader/cores
```

The Lua file should implement functions for searching and downloading anime. Here's an example script for the `nyaa` core:
```lua
local qbit = require("qbit")
local config = require("config")

config.default_config = {
    page_size = 10
}

local current_page = 1

local function get_torrent(torrents)
    local page = 1
    local option
    local page_size = config.get("page_size")

    while true do
        os.execute("clear")

        local total_torrents = #torrents
        local max_pages = math.ceil(total_torrents / page_size)
        page = math.max(1, math.min(page, max_pages))

        local start_index = (page - 1) * page_size + 1
        local end_index = math.min(start_index + page_size - 1, total_torrents)

        print("Page " .. page .. " of " .. max_pages)
        for i = start_index, end_index do
            local torrent = torrents[i]
            print(string.format("[%d]: %s (%s)", i, torrent.full_title, torrent.size))
        end

        print("\nOptions:")
        print("n - Next page")
        print("p - Previous page")
        print("<number> - Select torrent by number")
        print("q - Quit\n\nOption:")

        option = io.read()
        if option == "n" then
            if page < max_pages then
                page = page + 1
            end
        elseif option == "p" then
            if page > 1 then
                page = page - 1
            end
        elseif option == "q" then
            print("Exiting.")
            return nil
        elseif option:match("^(%d+)$") then
            local selected_index = tonumber(option)
            if selected_index >= 1 and selected_index <= total_torrents then
                return torrents[selected_index]
            end
            print("Invalid selection. Try again.")
        else
            print("Invalid input. Try again.")
        end
    end
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

    print("Downloading: " .. torrent.full_title .. " " .. torrent.size)
    qbit.download_magnet(torrent.magnet)
end

return {
    name = "nyaa",
    description = "Scrapes from https://nyaa.land then uses qbittorent to download.",
    version = "0.0.1",
    type = "torrent",
    download = download
}```
