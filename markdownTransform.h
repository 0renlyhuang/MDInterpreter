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
		"<h1 ", "<h2 ", "<h3 ", "<h4 ", "<h5 ", "<h6 ", // �ұߵļ�����Ԥ������������ı�ǩ����, �� id
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
	* ����Ŀ¼���
	*/
	Cnode(const string &hd) : heading(hd) {}
	vector<Cnode*> ch;
	string heading;
	string tag;
};

struct Node
{
	/*
	* �����������ݵĽṹ
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
	// �ж��Ƿ��Ǳ���
	bool isHeadding(Node *v) {
		return (v->type >= h1 && v->type <= h6);
	}

	// �ж��Ƿ�������
	bool isHref(Node *v) {
		return (v->type == href);
	}

	// �ж��Ƿ���ͼƬ
	bool isImage(Node *v) {
		return (v->type == image);
	}
	
	// �����������Ѱ�ҽڵ�	
	Node* findNode(int depth) {	
		Node *ptr = root;
		while (!ptr->ch.empty() && depth != 0) {
			ptr = ptr->ch.back(); // ???
			if (ptr->type == li)
				--depth;
		}
		return ptr;
	}

	// �ݹ����ٽڵ�
	template<typename T> 
	void destroy(T *v) {
		for (auto i : v->ch)
			destroy(i);
		delete v;
	}

	
	// Ŀ¼�����½ڵ� 
	void Cins(Cnode *v, int x, const string &hd, int tag);

	// ���Ĳ����½ڵ� ����ַ�����
	void insert(Node *v, const string &src);

	// �ж��Ƿ���
	bool isCutline(const string &src);

	// ���ɶ���
	void mkpara(Node *v);

	// �������Ľڵ� ����HTML�ַ���
	void dfs(Node *v);

	// ����Ŀ¼�ڵ㣬����HTML�ַ���
	void Cdfs(Cnode *v, string index);
	
	string _content, _table;
	Node *root = nullptr;
	Node *now = nullptr;
	Cnode *Croot = nullptr;
	string s;
	int cntTag = 0;
};



// ����ÿ�п�ʼ����һЩ�ո�� Tab
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
	 * �������ж� 
	 */
	string::const_iterator beg = src.cbegin(), it = beg;
	// it����#
	while (*it == '#')
		++it;

	// ���#֮���ǿո�
	if (it > beg && *it == ' ')
		return make_pair(it - beg + h1 - 1, src.substr(it - beg + 1));

	// ����п�ͷ��#��'''
	if (src.substr(0, 3) == "'''")
		return make_pair(blockcode, src.substr(3));

	// ����п�ͷ��*/+/-�ӿո�
	if (src.substr(0, 2) == "- " ||
		src.substr(0, 2) == "+ " ||
		src.substr(0, 2) == "* ")
		return make_pair(ul, src.substr(2));

	// ����п�ͷ��>�ӿո�
	if (src.substr(0, 2) == "> ")
		return make_pair(quote, src.substr(2));

	// ����п�ͷ�����ּ�.�ӿո�
	it = beg;
	while (isDigit(*it)) {
		++it;
	}
	if (it != beg && *it == '.' && *(it + 1) == ' ')
		return make_pair(ol, src.substr(it - beg + 2));

	// ���������ͨ����
	return make_pair(paragraph, src);
}




// ���ͻ�ȡ
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
