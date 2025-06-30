def shell_md(file_path):
    """
    Render a Markdown file as plain text in the terminal.

    This function reads a Markdown file from the given path, converts Markdown formatting into a simplified text representation, and prints the result to the terminal. It does not modify the original Markdown file and does not support editing within the preview.

    :param file_path: The path to the Markdown file to be rendered.
    """
    import markdown
    from markdown.extensions import extra, fenced_code
    from bs4 import BeautifulSoup

    # Read the Markdown file
    with open(file_path, 'r', encoding='utf-8') as file:
        md_content = file.read()

    # Convert Markdown to HTML
    html_content = markdown.markdown(md_content, extensions=[extra, fenced_code])

    # Convert HTML to plain text (strip tags, handle basic formatting)
    soup = BeautifulSoup(html_content, 'html.parser')
    plain_text = soup.get_text()

    # Print the plain text to the terminal
    print(plain_text)

