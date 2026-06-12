编译与运行
1. 编译（需要 C++17）
bash
g++ -std=c++17 -O2 replace_domain.cpp -o replace_domain
2. 使用示例
场景一：将所有链接变为相对路径（去掉域名）
bash
./replace_domain /home/ubuntu/galgamesource/www.ymgal.games
效果：https://www.ymgal.games/abc/def → /abc/def

场景二：替换为你服务器的公网地址（绝对 URL）
bash
./replace_domain /home/ubuntu/galgamesource/www.ymgal.games http://123.456.789.0:8080 --backup
效果：https://www.ymgal.games/abc/def → http://123.456.789.0:8080/abc/def

场景三：使用协议相对链接（避免 HTTPS/HTTP 混合）
bash
./replace_domain /home/ubuntu/galgamesource/www.ymgal.games //your-server.com
效果：https://www.ymgal.games/abc/def → //your-server.com/abc/def