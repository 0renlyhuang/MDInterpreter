#ifndef MARKDOWNTRANSFORM_H
#define MARKDOWNTRANSFORM_H

#include "helper.h"

#include <string>
#include <array>
#include <vector>
#include <utility>
#include <iostream>
#include <fstream>

using std::string;
using std::make_pair;
using std::pair;
using std::vector;
using std::array;

enum {
	maxLength = 10000,
	nul = 0,
	paragraph,
	href,
	ul,
	ol,
	li,
	em,
	strong,
	hr,
	br,
	image,
	quote,
	h1,
	h2,
	h3,
	h4,
	h5,
	h6,
	blockcode,
	code
};

const std::array<std::string, 20> frontTag{ {
		"", "<p>", "", "<ul>", "<ol>", "<li>", "<em>", "<strong>",
		"<hr color=#CCCCCC size=1 />", "<br />",
		"", "<blockquote>",
		"<h1 ", "<h2 ", "<h3 ", "<h4 ", "<h5 ", "<h6 ", // 右边的尖括号预留给添加其他的标签属性, 如 id
		"<pre><code>", "<code>"
} }; 

const std::array<std::string, 20> backTag = { {
	"","</p>","","</ul>","</ol>","</li>","</em>","</strong>",
	"","","","</blockquote>",
	"</h1>","</h2>","</h3>","</h4>","</h5>","</h6>",
	"</code></pre>","</code>"
} };

struct Cnode
{
	/*
	* 保存目录结果
	*/
	Cnode(const string &hd) : heading(hd) {}
	vector<Cnode*> ch;
	string heading;
	string tag;
};

struct Node
{
	/*
	* 保存正文内容的结构
	*/
	Node(int t) : type(t) {}
	int type;
	vector<Node*> ch;
	array<string, 3> elem;
};



class MarkdownTransform {
public:
	MarkdownTransform(const std::string&& filename);

	~MarkdownTransform() {
		destroy(root);
		destroy(Croot);
	}
	std::string getTable() const { return _table; } 
	std::string getContent() const { return _content; } 
private:
	// 判断是否是标题
	bool isHeadding(Node *v) {
		return (v->type >= h1 && v->type <= h6);
	}

	// 判断是否是链接
	bool isHref(Node *v) {
		return (v->type == href);
	}

	// 判断是否是图片
	bool isImage(Node *v) {
		return (v->type == image);
	}
	
	// 根据数的深度寻找节点	
	Node* findNode(int depth) {	
		Node *ptr = root;
		while (!ptr->ch.empty() && depth != 0) {
			ptr = ptr->ch.back(); // ???
			if (ptr->type == li)
				--depth;
		}
		return ptr;
	}

	// 递归销毁节点
	template<typename T> 
	void destroy(T *v) {
		for (auto i : v->ch)
			destroy(i);
		delete v;
	}

	
	// 目录插入新节点 
	void Cins(Cnode *v, int x, const string &hd, int tag);

	// 正文插入新节点 逐个字符解析
	void insert(Node *v, const string &src);

	// 判断是否换行
	bool isCutline(const string &src);

	// 生成段落
	void mkpara(Node *v);

	// 遍历正文节点 生成HTML字符串
	void dfs(Node *v);

	// 遍历目录节点，生成HTML字符串
	void Cdfs(Cnode *v, string index);
	
	string _content, _table;
	Node *root = nullptr;
	Node *now = nullptr;
	Cnode *Croot = nullptr;
	string s;
	int cntTag = 0;
};



// 处理每行开始处的一些空格和 Tab
inline pair<int, string> start(const string &src) {
	if (src.empty())
		return make_pair<int, string>(0, "");
	int spaceCnt = 0, tabCnt = 0;
	for (auto c : src) {
		if (c == ' ')
			++spaceCnt;
		else if (c == '\t')
			++tabCnt;
		else 
			return make_pair(tabCnt + spaceCnt / 4, src.substr(tabCnt + spaceCnt));		
	}
	return make_pair(0, "");
}

inline pair<int, string> judgeType(const string &src) {
	/*
	 * 行类型判断 
	 */
	string::const_iterator beg = src.cbegin(), it = beg;
	// it跳过#
	while (*it == '#')
		++it;

	// 如果#之后是空格
	if (it > beg && *it == ' ')
		return make_pair(it - beg + h1 - 1, src.substr(it - beg + 1));

	// 如果行开头无#是'''
	if (src.substr(0, 3) == "'''")
		return make_pair(blockcode, src.substr(3));

	// 如果行开头是*/+/-加空格
	if (src.substr(0, 2) == "- " ||
		src.substr(0, 2) == "+ " ||
		src.substr(0, 2) == "* ")
		return make_pair(ul, src.substr(2));

	// 如果行开头是>加空格
	if (src.substr(0, 2) == "> ")
		return make_pair(quote, src.substr(2));

	// 如果行开头是数字加.加空格
	it = beg;
	while (isDigit(*it)) {
		++it;
	}
	if (it != beg && *it == '.' && *(it + 1) == ' ')
		return make_pair(ol, src.substr(it - beg + 2));

	// 如果行是普通段落
	return make_pair(paragraph, src);
}




// 类型获取
inline bool isHeading(Node *v) {
	return (v->type >= h1 && v->type <= h6);
}

inline bool isImage(Node *v) {
	return (v->type == image);
}

inline bool isHref(Node *v) {
	return (v->type == href);
}
		






#endif 
