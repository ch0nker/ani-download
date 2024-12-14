# HTML
A library for parsing HTML.

## `html.parse(html)`
### Argument:
- `html` (string): The HTML to parse.

### Returns:
- `parser` (table):
    - `root` (light userdata): The root of the parsed document.
    - `xpath` (function): A function to querry the parsed document using XPath. (see `node.xpath` for more info)

## `node.xpath`
Evaluates the provided XPath expression on the parsed HTML document and returns a table of matching nodes.

### Arguments:
- `xpath` (string): The XPath expression to evaluate.

### Returns:
- (table): A table of nodes that match the XPath query. Each node has the following fields.
    - `name` (string): The node's name.
    - `text` (string): The node's text content.
    - `attributes` (table): A table of the node's attributes.
    - `children` (table): A table of child nodes.
    - `node_ptr` (light userdata): A pointer to the node.
    - `xpath` (function): The XPath query function that can be used on the current node.
