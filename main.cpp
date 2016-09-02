#include "markdownTransform.h"
#include <fstream>


int main() {
	MarkdownTransform transformer("test.md");

	string table = transformer.getTable();
	string content = transformer.getContent();

	std::string head = "<!DOCTYPE html><html><head>\
        <meta charset=\"utf-8\">\
        <title>Markdown</title>\
        <link rel=\"stylesheet\" href=\"markdown.css\">\
        </head><body><article class=\"markdown-body\">";
	std::string end = "</article></body></html>";

	std::ofstream out;
	out.open("D:\\Code\\c++\\MarkdwonParser\\index.html");
	out << head + table + content + end;
	out.close();
	return 0;
}