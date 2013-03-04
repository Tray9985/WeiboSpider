#pragma once
class DataStruct
{
public:
	DataStruct(void);
	~DataStruct(void);
	//微博正文
	CString Text;
	//发布时间
	CString Time;
	//来源
	CString From;
	//评论数
	unsigned int Comment;
	//转发数
	unsigned int Resport;
	//图片URL
	CString PicUrl;
	//是否转发
	bool IsResport;
	//是否带图片
	bool HasPic;
};

