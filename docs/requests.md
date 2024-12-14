# Requests
A library for making HTTP requests.

## `requests.make(data)`
Makes an HTTP request with custom settings.

### Arguments:
- `data`: (table): A table containing the following fields.
    - `url` (string): The URL for the request.
    - `method` (string): The HTTP method.
    - `headers` (table): A table containing the HTTP headers.
    - `?body` (string): The request body.

### Returns:
- `response` (table): A table containing the following fields.
    - `data` (string): The response body.
    - `headers` (table): The response headers.
    - `url` (string): The requested URL.
    - `status_code` (number): The HTTP status code of the response.
    - `status` (string): The status message from the response.
    - `json` (function): A function for parsing the response body as a table.

## `requests.get(data)`
Makes an HTTP GET request.

### Arguments:
- `data`: (table): A table containing the following fields.
    - `url` (string): The URL for the request.
    - `headers` (table): A table containing the HTTP headers.

### Returns:
Same as `requests.make`.

## `requests.post(data)`
Makes an HTTP POST request.

### Arguments:
- `data`: (table): A table containing the following fields.
    - `url` (string): The URL for the request.
    - `headers` (table): A table containing the HTTP headers.
    - `body` (string): The request body.

### Returns:
Same as `requests.make`.

## `requests.patch(data)`
Makes an HTTP PATCH request.

### Arguments:
- `data`: (table): A table containing the following fields.
    - `url` (string): The URL for the request.
    - `headers` (table): A table containing the HTTP headers.
    - `body` (string): The request body.

### Returns:
Same as `requests.make`.

## `requests.put(data)`
Makes an HTTP PUT request.

### Arguments:
- `data`: (table): A table containing the following fields.
    - `url` (string): The URL for the request.
    - `headers` (table): A table containing the HTTP headers.
    - `body` (string): The request body.

### Returns:
Same as `requests.make`.

## `requests.delete(data)`
Makes an HTTP DELETE request.

### Arguments:
- `data`: (table): A table containing the following fields.
    - `url` (string): The URL for the request.
    - `headers` (table): A table containing the HTTP headers.

### Returns:
Same as `requests.make`.

## `requests.url_encode(str)`
Encodes a string for use in a URL.

### Arguments:
- `str` (string): The string to encode.

### Returns:
A URL-encoded string.

## `requests.url_encode(str)`
Decodeds a URL-encoded string.

### Arguments:
- `str` (string): The URL-encoded string.

### Returns:
A decoded string.

