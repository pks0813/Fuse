###pksfuse
支持内容：模拟文件传输或聊天
假设person1想要向person2 传输内容

表示person2允许person1进行filename文件的传输
假设文件下若存在/person2/person1/filename
那么当person1在往目录/person1/person2/filename下进行write内容时会同时将内容传输到/person2/person1/filename
如果不存在则传输失败 但同样会往/person1/person2/filename下写内容

    >> mkdir pks1
    >> mkdir pks2
    >> mkdir pks1/pks2
    >> mkdir pks2/pks1
    >> echo "PKS1 send to PKS2" >> pks1/pks2/info
    >> cat pks1/pks2/info
    PKS1 send to PKS2
    >> cat pks2/pks1/info
    cat: pks2/pks1/info: No such file or directory
    >> touch pks2/pks1/Newinfo
    >> touch pks1/pks2/Newinfo
    >> echo "PKS2 send to PKS1">> pks2/pks1/Newinfo
    >> cat pks1/pks2/Newinfo
    PKS2 send to PKS1

实现思路：按照目录为key文件相关信息作为value构造一颗红黑树 用红黑树进行key,value对查找

