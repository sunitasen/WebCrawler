#define  CURL_STATICLIB
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <iterator>
#include <set>
#include <regex>
#include<typeinfo>
#include <queue>
#include<unordered_map>
#include<mysql.h>
#include "curl/curl.h"
int qstate;
std::unordered_map<std::string, int> urlList;


#ifdef _DEBUG
#pragma comment (lib , "curl/libcurl_a_debug.lib")
#else
#pragma comment (lib,"curl/libcurl_a.lib")
#endif 

std::string file_to_string(std::string file_name) {
	std::ifstream file(file_name);
	return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{} };
}

std::set<std::string> extract_hyperlinks(std::string html_file_name)
{
	static const std::regex hl_regex("href=\"(.*?)\"", std::regex_constants::icase);

	const std::string text = file_to_string(html_file_name);

	return { std::sregex_token_iterator(text.begin(), text.end(), hl_regex, 1),
			 std::sregex_token_iterator{} };
}

std::string extract_domain(std::string urlName)
{
	std::regex expression("/^(http://|https://)[A-Za-z0-9.-]+(?!.*\|\w*$)/");
	std::regex re("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
	std::smatch match;
	std::string str(urlName);
	std::string val = urlName;
	std::string retVal1,retVal2,retVal;
	if (regex_match(val, match, re)) {
		retVal1 = (match[1]);
		retVal2 = (match[2]);
	}
	retVal = retVal1 + "://" + retVal2;
	return retVal;
	
}


static int writer(char *data, size_t size, size_t nmemb, std::string *writerData) {
	if (writerData == NULL)
		return 0;

	writerData->append(data, size*nmemb);

	return size * nmemb;
}

void showq(std::queue <std::string> gq){
	std::queue <std::string> g = gq;
	while (!g.empty())
	{
		std::cout << '\n' << g.front();
		g.pop();
	}
	std::cout << '\n';
}

int main()
{
	MYSQL *conn;
	MYSQL_ROW row;
	MYSQL_RES *res;
	conn = mysql_init(NULL);
	conn = mysql_real_connect(conn, "127.0.0.1", "root", "password", "crawler", 3306, NULL, 0); // "localhost" fails 

	if (conn) {
		puts("succesfully connected to the database");

	}
	else {
		puts("cannot connect");
	}


	std::string fetchedUrl;
	int fetchedUrlLinked;
	std::string content;
	std::string filename = "C:\\Users\\Admin\\junk\\newfile.html";
	std::queue<std::string> urls;
	std::string start = "https://github.com/sunitasen";

	std::string query = "select name,linked from urls";
	const char *q = query.c_str();
	qstate = mysql_query(conn, q);
	if (!qstate) {
		res = mysql_store_result(conn);
		while (row = mysql_fetch_row(res)) {
			fetchedUrl = row[0];
			fetchedUrlLinked = atoi(row[1]);
			urlList[fetchedUrl] = fetchedUrlLinked;
		}
	}
	else {
		std::cout << "query failed" << mysql_error(conn) << "\n";
	}



	urls.push(start);
	int i = 0;


	while (i < 10) {
		curl_global_init(CURL_GLOBAL_ALL);
		std::ofstream fp;
		std::string starturl = urls.front();
		std::string str(starturl);
		std::string domainName = extract_domain(starturl);
		const char *parUrl = starturl.c_str();
		std::cout << starturl << "\n";
		CURL *curl = nullptr;
		curl = curl_easy_init();
		if (curl) {
			fp.open(filename, std::ios::out);
			curl_easy_setopt(curl, CURLOPT_URL, parUrl);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
			CURLcode code = curl_easy_perform(curl);
			curl_easy_cleanup(curl);
		}
		curl_global_cleanup();
		fp << content;
		
		for (auto str : extract_hyperlinks(filename)) {
			std::cout << str << '\n';
			if (str[0] == 'h') {
				if (urlList.find(str) == urlList.end()) {
					urls.push(str);
					urlList[str] = 1;
					std::string queryinit = "insert into urls values( \"";
					queryinit += str + "\" , 1)";
					qstate = mysql_query(conn, queryinit.c_str());
					if (!qstate) std::cout << "done!";
					else std::cout << "nope";

				}
				else {
					std::string query = "select name,linked from urls where name=\"";
					query += str + "\"";
					mysql_query(conn, query.c_str());
					res = mysql_store_result(conn);
					row = mysql_fetch_row(res);
					fetchedUrlLinked = atoi(row[1]);
					std::string queryinit = "update urls set linked =";
					fetchedUrlLinked += 1;
					urlList[str] = fetchedUrlLinked;
					queryinit += std::to_string(fetchedUrlLinked) + " where name = \"" + str + "\"";
					qstate = mysql_query(conn, queryinit.c_str());
					if (!qstate) std::cout << "updated!\n";
					
				}

			}
			else if(str[0] == '/'){
				std::string subUrl = domainName + str;
				if (urlList.find(subUrl) == urlList.end()) {
					urls.push(subUrl);
					urlList[subUrl] = 1;
					std::string queryinit = "insert into urls values( \"";
					queryinit += subUrl + "\" , 1)";
					qstate = mysql_query(conn, queryinit.c_str());
					if (!qstate) std::cout << "worked";
					else std::cout << "nope";
				}
				else {
					std::string query = "select name,linked from urls where name=\"";
					query += subUrl + "\"";
					mysql_query(conn, query.c_str());
					res = mysql_store_result(conn);
					row = mysql_fetch_row(res);
					fetchedUrlLinked = atoi(row[1]);
					std::string queryinit = "update urls set linked =";
					fetchedUrlLinked += 1;
					urlList[subUrl] = fetchedUrlLinked;
					queryinit += std::to_string(fetchedUrlLinked) + " where name = \"" + subUrl + "\"";
					qstate = mysql_query(conn, queryinit.c_str());
					if (!qstate) std::cout << "updated";

				}
			}

		}
		
		filename = "C:\\Users\\Admin\\junk\\file" + std::to_string(i) + ".html";
		urls.pop();

		fp.close();
		i++;
	}
	std::cout << "end reached";
	return 0;
}

