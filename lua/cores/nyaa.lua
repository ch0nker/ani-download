local qbit = require("qbit")
local config = require("config"):new("nyaa", {
    page_size = 10
})

ui.init()
ui.set_cursor_type(0)

local nyaa = {}

function nyaa.search(title, page)
    local response = requests.get({
        url = "https://nyaa.land/?f=0&c=1_2&p=" .. tostring(page) .. "&q=" .. (requests.url_encode(title) or "")
    })

    local parser = html.parse(response.data)
    local torrent_elms = parser:xpath("//tbody")[1]
    local amount = parser:xpath("//div[@class=\"pagination-page-info\"]")[1].text:match("out of (%d+) results.")
    local max_page = parser:xpath("//ul[@class=\"pagination\"]/li[not(@class=\"next\") and not(contains(@class, \"disabled\"))][last()]")[1].text
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

    return torrents, tonumber(amount), tonumber(max_page)
end


function nyaa.get_torrent(title, page)
    local start_index
    local result

    local actual_page = page
    local page_size = math.min(config.page_size, ui.rows - 6)
    local torrents, amount, max_page = nyaa.search(title, page)

    local total_torrents = #torrents
    local max_pages = math.ceil(amount / page_size)

    local current_window = "search"
    local control_window
    local window = ui.create({
        width = ui.cols,
        height = page_size + 5
    })

    local x, y = 1, 1
    local space = 2
    local rows

    ui.init_color("select", ui.color.black, ui.color.white)
    ui.init_color("normal", ui.color.white, ui.color.black)
    ui.init_color("leech", ui.color.red, ui.color.black)
    ui.init_color("seed", ui.color.green, ui.color.black)

    window:set_color("normal")

    local function print_lines()
        local width = window.width - 2
        local line = "├" .. string.rep("─", width) .. "┤"
        local help_text = "Press ? for controls"

        window:print(0, 2, line)
        window:print(window.width - #help_text - 2, 1, help_text)
    end

    local function show_controls()
        current_window = "controls"

        local lines = {
            "Enter: Download the selected post.",
            "c: Opens the comments for the selected post.",
            "n: Go to the next page.",
            "p: Go to the previous page",
            "?: Opens this window.",
            "UP-DOWN: Scroll the posts.",
            "LEFT-RIGHT: Scroll the post's title."
        }

        local width = 46
        local height = #lines + 2
        width = window.width < width and window.width or width
        height = window.height < height and window.height or height

        control_window = ui.create({
            width = width,
            height = height,
            x = (window.width - width) / 2,
            y = (window.height - height) / 2
        })
        control_window:clear()

        for i, line in next, lines do
            control_window:print(1, i, line)
        end
    end

    local function print_torrent(torrent, line, selected)
        local leech_text = string.format("%d", torrent.leechers)
        local seed_text = string.format("%d", torrent.seeders)

        local info_text = string.format(" [%d] [%d]", seed_text, leech_text)

        window:print(window.width - (#info_text + 1), line + space, info_text)
        window:set_color("leech")
        window:print(window.width - (#leech_text + 2), line + space, leech_text)
        window:clear_color()
        window:set_color("seed")
        window:print(window.width - (#leech_text + 5 + #seed_text), line + space, seed_text)
        window:clear_color()

        if selected then
            window:set_color("select")
            window:print(1, line + space, string.format("%s (%s)", torrent.full_title:gsub("[^\x00-\x7F]", ""):sub(x, x + (window.width - #info_text - (#torrent.size + 5))), torrent.size))
            window:clear_color()
        else
            window:set_color("normal")
            window:print(1, line + space, string.format("%s (%s)", torrent.full_title:gsub("[^\x00-\x7F]", ""):sub(1, window.width - #info_text - (#torrent.size + 5)), torrent.size))
            window:clear_color()
        end
    end

    local function output_page()
        window:clear()

        page = math.max(1, math.min(page, max_pages))
        start_index = (page - 1) * page_size + 1
        print_lines()

        if start_index + page_size - 1 > total_torrents and page <= max_pages and actual_page < max_page then
           actual_page = actual_page + 1
           local search_text = string.format("Loading Page: %d..", actual_page)
           window:print((window.width - #search_text) / 2, 1, search_text)

           local new_torrents, new_amount, new_max_page = nyaa.search(title, actual_page)
           for _, v in next, new_torrents do
               table.insert(torrents, v)
           end
           total_torrents = #torrents
           amount = new_amount
           max_page = new_max_page
           window:clear()
           print_lines()
        end

        local end_index = math.min(start_index + page_size, total_torrents)
        local page_title = string.format("Page: %d/%d", page, max_pages, actual_page, max_page)
        window:print(2, 1, page_title)
        rows = 1
        for i = start_index, end_index do
            local torrent = torrents[i]

            print_torrent(torrent, rows, rows == y)

            rows = rows + 1
        end
    end

    output_page()

    ui.on_key(function (key)
        if key == "q" then
            if current_window == "controls" then
                control_window:destroy()
                output_page()
                current_window = "search"
                return true
            end
            return false
        end

        if current_window == "comments" then
            return true
        end

        if current_window ~= "search" then
            return true
        end

        if key == "n" then
            page = page + 1
            x = 1
            y = 1
            output_page()
            return true
        elseif key == "\n" then
            result = torrents[start_index + y - 1]
            return false
        elseif key == "p" then
            page = page - 1
            x = 1
            y = 1
            output_page()
            return true
        elseif key == "?" then
            show_controls()
            return true
        end

        local torrent = torrents[start_index + y - 1]

        print_torrent(torrent, y, false)

        if key == "UP" then
            y = y == 1 and rows - 1 or math.max(1, y - 1)
            x = 1
        elseif key == "DOWN" then
            y = y == rows - 1 and 1 or math.min(y + 1, rows - 1)
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

        print_torrent(torrent, y, true)

        return true
    end)

    window:destroy()
    return result
end

local function download(title)
    print(string.format("Searching for %s..", title))

    local torrent = nyaa.get_torrent(title, 1)

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
