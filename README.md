# Linux_Command
# Thư mục root
Cơ sở của hệ thống phân cấp tệp Linux bắt đầu từ thư mục gốc (root).
Thư mục root thường được gọi là "dấu gạch chéo /" vì không có thư mục nào khác phía trên nó,nếu ai đó nói rằng hãy nhìn kỹ vào "dấu gạch chéo" hoặc tệp đó ở trong "dấu gạch chéo", thì họ đang đề cập đến thư mục root.

# Thư mục /bin
Thư mục chứa mã máy của các chương trình sau khi biên dịch

# Thư mục /etc
Thư mục chứa các tệp cấu hình của các chương trình,phần mềm trong Linux.

# Thư mục /home
Là nơi lưu trữ dữ liệu của người dùng, một máy tính có thể có nhiều người dùng,đây là nơi để lưu trữ và tách dữ liệu giữa các người dùng khác nhau.

# Thư mục /opt
Đây là nới chứa các phần mềm bên thứ ba không đi kèm với hệ điều hành.

# Thư mục /tmp
Là không gian tạm thời, sẽ được clear khi khởi động, vì vậy nếu bạn lưu trữ file trong đây thì khi khởi động nó sẽ được xóa đi.

# Thư mục /usr
Thư mục chứa các chương trình liên quand đến người dùng

# Thư mục /var
Thư mục này chứa các dữ liệu thường xuyên thay đổi như các tệp nhật ký.

# Thư mục /boot
Thư mục chứa các tệp khởi động của hệ điều hành, đây là nới bạn sẽ tìm thấy nhận linux

# Thư mục /media
Đây là thư mục để truy cập các tệp được lưu trữ trong CD Rom,DVD và USB cắm vào máy tính

# Thư mục /export
Chia sẻ các file hệ thống

# Thư mục /mnt
Là nơi để gắn kết các file hệ thống bên ngoài.

# Thư mục /root
Khác với thư mục root

The Prompt
# [jason@linuxsvr ~]$ 
Đây là thể hiện người dùng tên là jason theo là đấu @ và tên của hệ thống linux đang kết nối linuxsvr
Ở cuối lời nhắc, bạn sẽ thấy ký hiệu đô la $, đây là dấu hiệu cho thấy bạn đang sử dụng hệ thống với tư cách là người dùng bình thường chứ không phải là người dùng cấp cao.

# [root@linuxsvr:~]#
Đây thể hiện là người dùng cấp cao và root ở đây là tên người không liên quan đến thư mục root
# jason@linuxsvr:~>
Lưu ý dấu ~ biểu thị cho thư mục chính của tài khoản hiện tại, ví dụ 
    ~jason = /home/jason
    ~pat   = /home/pat
    ~root  = /root
    ~ftp   = /var/ftp

Commands Linux
Điều đầu tiên bạn cần biết là Linux commands phân biệt chữ hoa và chữ thường

# pwd
Hiển thị đường dẫn thư mục làm việc hiện tại

# cd [dir]
Di chuyển bạn đến thư mục dir, nếu không có dir thì sẽ lùi lại thư mục mẹ
Note:   cd . Thư mục hiện hành
        cd .. Thư mục mẹ
        cd - Trở lại thư mục trước đó

# ls [OPTION]
Lệnh liệt kê các tệp và thư mục hiện có trong thư mục hiện tại
Note:   ls -l Liệt kê thông tin các tệp
        ls -lr Liệt kê thông tin các tệp theo chiều ngược lại
        ls -a hoặc ls -l -a Liệt kê các file ẩn
        ls --color hiển thị danh sách các tệp và thư mục với   các màu sắc khác nha
        ls -R liệt kê tất cả tệp và thư mục con trong thư mục hiện tại
        ls -F giúp bạn nhận biết loại tệp hoặc thư mục
              Các ký hiệu:
                / Thêm vào cuối tên thư mục.
                * Thêm vào cuối tên tệp thực thi.
                @ Thêm vào cuối tên liên kết mềm (symlink).
                | Thêm vào cuối tên các tệp FIFO (First In First Out).
                = Thêm vào cuối tên của socket.
                VD: bin/  script.sh*  link@  fifo|  socket=
        ls -l "my notes.txt" mở một tệp có khoảng trắng
		ls --color
Note: 

# tree [OPTION]
Lệnh này hiển thị các tệp và thư mục hiện có trong thư mục chính ở dạng cây thư mục
Note: tree hiển thị tất cả tệp và thư mục
      tree -d chỉ hiển thị thư mục
      tree -C giống lệnh tree nhưng có tô màu sắc dễ nhìn


# cat [OPTION] [FILE]...
Lệnh cat cho phép người dùng tạo một hoặc nhiều file, xem nội dung file, nối file và chuyển hướng đầu ra trong terminal hoặc file.
    + Hiển thị nội dung của file
        cat /etc/hosts
    + Xem nội dung của nhiều file trong Terminal
        cat test test1
    + Tạo file bằng lệnh Cat
        cat >test2
        sau đó nhập văn bản vào và nhấn CTRL + D
    + Sử dụng lệnh Cat với các tùy chọn more và less
        cat song.txt | more hiển thị nhiều nội dung tệp hơn
        cat song.txt | less hiển thị ít nội dung tệp hơn
    + Hiển thị số dòng trong file
        cat -n song.txt
    + Hiển thị $ ở cuối file
        cat -e test
    + Hiển thị các dòng được phân tách bằng tab trong file
        cat -T test
    + Hiển thị nhiều file cùng một lúc
        cat test; cat test1; cat test2
    + Ghi đè nội dung từ một file vào một file khác
        cat test > test1
    + Ghi thêm nội dung vào một file khác
        cat test >> test1
    + Chuyển hướng đầu vào chuẩn với toán tử chuyển hướng
        cat < test2
        Thay vì cung cấp tệp trực tiếp cho lệnh cat (như cat test2), lệnh cat < test2 lấy đầu vào từ tệp test2 và hiển thị nội dung đó ra màn hình. Tuy nhiên, trong trường hợp này, kết quả đầu ra sẽ giống hệt với lệnh cat test2
    + Ghi đè nội dung nhiều file vào một tệp
        cat test test1 test2 > test3
    + Ghi thêm nội dung nhiều file vào một tệp
        cat test test1 test2 >> test3
    + Sắp xếp nội dung của nhiều file trong một file duy nhất
        cat test test1 test2 test3 | sort > test4
    
    
# ENVIROMENT VARIABLES
Chúng là các biến môi trường chứa các thông tin nào đó
Thông thường các biến này sẽ in hoa tất cả chữ cái
Thông thường để in ra giá trị của chúng ta có thể dùng lệnh "echo $VAR_NAME"

# echo $PATH
PATH là một biến môi trường chứa danh sách đường dẫn đến các thư mục mà hệ thống sẽ tìm kiếm khi một lệnh được gọi hoặc một chương trình được thực thi. Khi bạn chạy một lệnh trong Linux mà không chỉ rõ đường dẫn tuyệt đối (như /bin/ls), hệ thống sẽ tìm kiếm chương trình thực thi của lệnh này trong các thư mục được liệt kê trong biến PATH
Giả sử bạn chạy lệnh echo $PATH
Kết quả: /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games
Danh sách này cho hệ thống biết rằng khi bạn nhập một lệnh, nó sẽ tìm kiếm lệnh đó theo thứ tự trong các thư mục này. Nếu tìm thấy lệnh trong một thư mục nào đó, nó sẽ thực thi lệnh từ thư mục đó và không tiếp tục tìm trong các thư mục còn lại

# which [command]
Cho biết thư mục chứa chương trình để thực hiện lệnh này

# mkdir [p] directory
Tạo một thư mục rỗng hoặc một thư mục với nhiêu thư mục con bên trong

# rmdir [p] directory
Remove một thư mục

# rm -rf directory
Xóa thư mục và tất cả tệp trong thư mục 
rm -rf là lệnh nguy hiểm, đặc biệt khi được sử dụng với các thư mục hệ thống hoặc sai đường dẫn, vì nó xóa mọi thứ mà không thể lấy lại được


# File and Directory Permissions 
VD: drwxrwxr-x 3 datlt49 datlt49 4096 Thg 11 20 10:31 build 
	-rw-rw-r-- 1 datlt49 datlt49 64 Thg 11 18 10:43 Build_and_Run.sh 
	-rw-rw-r-- 1 datlt49 datlt49 962 Thg 11 18 10:43 CMakeLists.txt 
	drwxrwxr-x 4 datlt49 datlt49 4096 Thg 11 18 10:20 
	Core Phân tích:   
	+Ký tự đầu tiên:     
	"d" : là thư mục     
	"-" : là tệp binh thường     
	"|" : là một loại liên kết     
	Nếu là thiết bị đặc biệt ký tự đầu tiên có thể là "b" hoặc "c"   
	+Thông tin quyền truy cập:   
	VD: "-rw-rw-r-- 1 datlt49 datlt49"     
		Với "rw-rw-r--":       
			"rw-": quyền của chủ sở hữu (đọc-ghi)       
			"rw-": quyền của nhóm người dùng (đọc-ghi)       
			"r--": quyền của người dùng khác (chỉ đọc)     
			Với "datlt49 datlt49":       
				"datlt49": tên người dùng       
				"datlt49": tên nhóm người dùng     
				Với "1" : Số lượng liên kết đến với tệp này.   
	VD: "drwxrwxr-x 4 datlt49 datlt49"     
		Với "rwxrwx":       
			"rwx"  : quyền của chủ sở hữu (đọc-ghi-thựcthi)       
			"rwx"  : quyền cùa nhóm người dùng (đọc-ghi-thựcthi)       
			"r-x"  : quyền của người dùng khác có quyền đọc (r) và thực thi (x), nhưng không có quyền ghi (w). 


# Change Permissions 
VD: chnmod g+wx sales.data 
	chnmod u+rwx,g-x sales.data 
	chnmod a=r sales.data 
	chnmod u=rw, g=rx, o=sales.data 
	Lệnh: 
		"chnmod" : command thay đổi permissions 
		"u"      : user 
		"g"      : group 
		"o"      : other 
		"a"      : all 
		"+"      : add 
		"-"      : remove 
		"="      : set 
		"rwx"   : read,write,excute 
	Change mode table:  
		OCTAL  BIN    String    Description    
		  0     0     ---    	No permissions    
		  1     1     --x    	Execute only    
		  2     10    -w-    	Write only    
		  3     11    -wx    	Write Execute    
		  4     100   r--    	Read Execute    
		  5     101   r-x    	Read Execute    
		  6     110   rw-    	Read Write    
		  7     111   rwx    	Read Write Execute 
	  
VD: chnmod 777 sales.data <=> chnmod u+rwx, g+rwx, o+rwx sales.data 

"Note" 
chmod -R u+rwx,g+rwx,o+rwx /home/datlt49/Desktop/02_Github_ENV/DatLT49 
=> thay đổi tất cả các đối tượng trong thư mục nguồn đều có thể rwx


# find item 
find [path] [command]   
"find" : Tìm tất cả các file trong thư mục hiện hành   
"find -mtime days"  :   
"find -size num"  :   
"find -newer file"  :   
"exec "         

# Xem và chỉnh sửa files 
+ "cat file" 	: Display the contents of file. 
+ "more file" 	: Browse through a text file. 
+ "less file" 	: More features than more. 
+ "head file" 	: Output the beginning (or top)         portion of file. 
+ "tail file" 	: Output the ending (or bottom)         portion of file. 
+ "rm file" 	: Remove file. 
+ "rm -r dir" 	: Remove the directory and its        contents recursively. 
+ "rm -f file"	: Force removal and never        prompt for confirmation 

# Copy command 
+ "cp source_file destination_file" : Copy source_file to destination_file. 
+ "cp src_file1 [src_fileN ...] dest_dir" : Copy source_files to destination_directory. 
+ "cp -i [tệp nguồn] [thư mục đích]"   : Kiểm tra xem tệp nguồn đã tồn tại ở thư mục đích hay chưa, nếu rồi hệ thống sẽ bắt xác nhận ghi đè hay không 
+ "cp -r source_directory destination"    : Copy src_directory recursively to destination. 

# Move or rename files 
+ "mv source destination" : tương tự copy nhưng là move 
+ "mv -i source destination": tương tự copy nhưng là move # Kiểm tra dung lượng được sử dụng ->du (Disk Usage) 
+ "du" : Estimates file usage 
+ "du -sh": -s: Tóm tắt, chỉ hiển thị dung lượng của thư mục chính (không liệt kê từng thư mục con). -h: Hiển thị dung lượng theo đơn vị dễ hiểu (KB, MB, GB, v.v.). 
+ "du -ah": Để liệt kê dung lượng của tất cả các thư mục và các thư mục con (từ trong ra ngoài) 
+ "du -k" : Display sizes in Kilobytes 
+ "du -h" : Hiển thị dung lượng của tất cả các thư mục con 

# Kiểm tra dung lượng trống ->df (Disk Free) 
+ "df" : Lệnh này sẽ hiển thị dung lượng của các phân vùng đĩa (cả đã sử dụng và còn trống) trên hệ thống, tính bằng kilobytes (KB). 
+ "df -k" : Display sizes in Kilobytes 
+ "df -h" : Hiển thị dung lượng theo đơn vị dễ đọc (KB, MB, GB, v.v.) 
+ "df -a" : Hiển thị tất cả các phân vùng

# Creating a Collection of Files
Lệnh tar là một công cụ được sử dụng để đóng gói nhiều tệp tin hoặc thư mục lại với nhau thành một tập tin nén (archive). 
Tuy nhiên, tar không thực hiện nén dữ liệu. Mục đích chính của tar là gộp nhiều tệp tin thành một tập tin duy nhất, 
giúp dễ dàng quản lý và sao lưu. 
tar [-] c|x|t f tarfile [pattern]
Trong đó:
	+ "c" : Create – Tạo một tập tin nén mới (archive). Đây là tùy chọn được sử dụng khi bạn muốn đóng gói các tập tin hoặc thư mục lại với nhau.
	+ "x" : Extract – Giải nén (extract) các tập tin từ một tập tin nén (archive). Đây là tùy chọn được sử dụng khi bạn muốn giải nén dữ liệu từ một tập tin nén.
	+ "t" : List – Liệt kê các nội dung trong một tập tin nén mà không giải nén nó. Đây là tùy chọn dùng để xem các tập tin có trong một tập tin nén.
	+ "f" : File – Xác định tên của tập tin nén (archive file). Đây là tùy chọn bắt buộc nếu bạn muốn chỉ định tên tập tin nén trong lệnh tar. Sau tùy chọn này, bạn sẽ cần cung cấp tên của tập tin nén.
	+ "tarfile" : Tên của tập tin nén (archive file) mà bạn muốn thao tác (tạo, giải nén, hoặc liệt kê).
	+ "pattern" : (Tùy chọn) Là mẫu (pattern) của các tập tin bạn muốn chọn khi tạo hoặc giải nén từ tập tin nén. Mẫu có thể là tên tập tin, thư mục hoặc "wildcard" như *, *.txt, ...
VD:
	Tạo một tập tin nén : 
	"tar -cf archive.tar file1 file2 file3"
	Giải nén một tập tin nén : 
	"tar -xf archive.tar"
	Liệt kê các nội dung trong tập tin nén : 
	"tar -tf archive.tar"
	Tạo tập tin nén với một mẫu (pattern) :
	"tar -xf archive.tar *.txt"

# Compressing Files To Save Space
Sau khi dùng tar để gộp file lại thành một tập tin duy nhất ta có thể dùng gzip và các câu lệnh liên quan
để nén dung lượng

Ví dụ:
	# gzip
	+ "gzip file.txt" : Sau khi thực hiện lệnh trên, tệp tin file.txt sẽ được nén thành file.txt.gz.
	+ "gzip file1.txt file2.txt" : Lệnh trên sẽ nén file1.txt và file2.txt thành file1.txt.gz và file2.txt.gz.
	Tùy chọn:
		"-d": Giải nén tệp tin .gz (tương đương với lệnh "gunzip").
		"-k": Giữ lại tệp tin gốc sau khi nén (mặc định gzip sẽ xóa tệp gốc sau khi nén).
	
	# gunzip
	+ "gunzip file.txt.gz" : Sau khi thực hiện lệnh trên, tệp tin file.txt.gz sẽ được giải nén thành file.txt.
	+ "gunzip file1.txt.gz file2.txt.gz" : Giải nén nhiều tệp
	Tùy chọn:
		"-c" : Giải nén và xuất nội dung ra stdout thay vì ghi vào tệp.
		"-v" : Hiển thị thông tin chi tiết khi giải nén.
		"-d" : Tương đương với gunzip, dùng để giải nén
	
	# gzcat
	gzcat được sử dụng để xem nội dung của các tệp tin nén .gz mà không cần phải giải nén chúng hoàn toàn.

	# zcat
	Tương tự gzcat, xem nội dung của tệp tin .gz
	
	
# Wildcards
Wildcards trong Linux là các ký tự đặc biệt được sử dụng trong dòng lệnh để khớp mẫu (pattern matching) 
với tên tệp và thư mục. Chúng giúp thực hiện các tác vụ như tìm kiếm, liệt kê, sao chép hoặc xóa tệp 
mà không cần chỉ định chính xác tên từng tệp

	# * Dấu sao - Asterisk
	+ Khớp với bất kỳ chuỗi ký tự nào, bao gồm cả chuỗi rỗng.
	Ví dụ:
		"ls *.txt" : Liệt kê tất cả các tệp có phần mở rộng .txt trong thư mục hiện tại (ví dụ: file1.txt, doc.txt)
		"rm -rf mydir/*" : Xóa tất cả các tệp và thư mục con bên trong mydir.
	
	# ? Dấu chấm hỏi - Question Mark
	+ Khớp với một ký tự bất kỳ duy nhất.
	Ví dụ: 
		"ls file?.txt" : Liệt kê tất cả các tệp có tên bắt đầu bằng "file", 
						tiếp theo là một ký tự duy nhất, và kết thúc bằng .txt 
						(ví dụ: file1.txt, fileA.txt, nhưng không khớp với file12.txt).
		
	# [ ] Square Brackets - Dấu ngoặc vuông
	+ Khớp với một ký tự bất kỳ trong danh sách được đặt trong dấu ngoặc vuông.
	+ Phạm vi ký tự cũng có thể được chỉ định.
	Ví dụ:
		"ls file[1-3].txt" : Liệt kê các tệp file1.txt, file2.txt, và file3.txt
		"ls file[abc].txt" : Khớp với filea.txt, fileb.txt, hoặc filec.txt.
	
	# [! ] hoặc [^ ] (Dấu phủ định)
	+ Khớp với bất kỳ ký tự nào ngoại trừ các ký tự được chỉ định trong dấu ngoặc vuông.
	Ví dụ:
		"ls file[!a-z].txt" : Liệt kê các tệp có tên file theo sau là bất kỳ ký tự nào không phải là chữ 
							cái thường từ a đến z, chẳng hạn như file1.txt, file9.txt.
							
	# {} Curly Braces - Dấu ngoặc nhọn
	+ Dùng để mở rộng danh sách các mục và chạy nhiều lệnh trong một lần.
	Ví dụ:
		"mkdir {backup,archive,logs}" : Tạo ba thư mục: backup, archive, và logs.
		"mv file{1,2,3}.txt /backup"  : Di chuyển file1.txt, file2.txt, và file3.txt vào thư mục /backup
		
Ứng dụng:
	+ " find /home/user -name "*.log" " : Tìm tất cả các tệp .log trong /home/user.
	+ " cp *.jpg /backup/images " : Sao chép tất cả các tệp .jpg vào /backup/images.
	+ " rm file[1-9].txt " : Xóa tất cả các tệp file1.txt đến file9.txt.

Lưu Ý Khi Sử Dụng Wildcards
	+ Wildcards chỉ áp dụng cho tên tệp và thư mục, không áp dụng cho nội dung tệp.
	+ Wildcards hoạt động với Shell của Linux (như bash, sh, zsh).
	+ Để vô hiệu hóa Wildcards, sử dụng dấu gạch chéo ngược \ 
	Ví dụ: 
		" find /home/user -name "file\*name.txt" " : Tìm tệp có tên chứa dấu sao * như file*name.txt
		" find /home/user -name "file\?name.txt" " : Tìm tệp có tên chứa dấu chấm hỏi ? như file?name.txt

# Input/Output 
Linux có 3 loại luồng dữ liệu chính ( "Standard Streams" ):

"I/O Name" 			"Abbreviation" 	"File Descriptor"
Standard Input 			stdin 				0
Standard Output	 		stdout 				1
Standard Error 			stderr 				2
=> Đây cũng là các File Descriptor Mặc Định


	# Chuyển hướng Đầu ra (Output Redirection)
	
		+ ">"  Ghi đè đầu ra vào một tệp. Nếu tệp đã tồn tại, nội dung cũ sẽ bị ghi đè.
		Ví dụ:
			" echo "Hello, Linux!" > output.txt "   : Tạo hoặc ghi đè nội dung "Hello, Linux!" vào tệp output.txt

		+ ">>" Ghi thêm đầu ra vào cuối tệp mà không ghi đè.
		Ví dụ:
			" echo "This is a new line." >> output.txt " : Thêm dòng "This is a new line." vào cuối tệp output.txt
		
		+ "2>" Gửi lỗi của lệnh vào một tệp.
		Ví dụ:
			" ls nonexistentfile 2> error.log " : Ghi thông báo lỗi vào error.log khi tệp không tồn tại.
		
		+ "2>>" Ghi thêm thông báo lỗi vào tệp.
		Ví dụ:
			" ls nonexistentfile 2>> error.log " : Thêm thông báo lỗi vào cuối tệp error.log nếu tệp không tồn tại.
		
		+ "&>" Ghi cả đầu ra và lỗi vào một tệp. "&" được sử dụng để báo hiệu rằng một trình mô tả tệp ( "File descriptor" ) đang được sử dụng.
		Ví dụ:
			" ls /nonexistentpath &> output.log " : Ghi cả thông báo lỗi và đầu ra vào output.log.

		+ ">&" Gửi cả stdout và stderr vào một tệp hoặc thiết bị.
		Ví dụ:
			" command > output.txt 2>&1 " : Ghi cả đầu ra và lỗi vào output.txt.

	# Chuyển hướng Đầu vào (Input Redirection)
	Tương tự như Output
	Ví dụ:
		" wc -l < input.txt " : Đếm số dòng trong tệp input.txt bằng cách đọc tệp làm đầu vào.

	# Chuyển hướng Nối Dữ Liệu (Piping)
	
	+ "|" (Dấu ống) Dùng để nối đầu ra của lệnh trước làm đầu vào cho lệnh sau.
	Ví dụ:
		" cat file.txt | grep "Linux" " : Tìm kiếm từ "Linux" trong tệp file.txt.
		
	# Chuyển hướng Vô hiệu hóa (Null Device)

	+ "/dev/null" Dùng để bỏ qua đầu ra hoặc loại bỏ lỗi.
	Ví dụ:
		" ls nonexistentfile 2> /dev/null " 	Bỏ qua thông báo lỗi khi tệp không tồn tại.
		" ls here not-here > /dev/null 2>&1 "   Bỏ qua cả thông báo lỗi và đầu ra khi tệp không tồn tại.
		" ./script.sh > /dev/null "				Bỏ qua đầu ra nhưng giữ thông báo lỗi
		" ./script.sh 2> /dev/null "			Bỏ qua thông báo lỗi nhưng giữ đầu ra

# File Descriptor
File Descriptor (FD) trong Linux là một số nguyên không âm đại diện cho một tệp hoặc tài nguyên được mở
bởi "một tiến trình". Nó là một cơ chế trừu tượng giúp hệ điều hành và chương trình quản lý tệp, thiết bị 
đầu vào/đầu ra, hoặc socket mạng mà không cần quan tâm đến cách thức hoạt động nội bộ của thiết bị.

Cách Hoạt Động của File Descriptor
+ Khi một tệp, socket, hoặc thiết bị được mở, hạt nhân (kernel) gán cho nó một số nguyên gọi là File Descriptor.
+ Chương trình sử dụng số nguyên này để tương tác với tài nguyên tương ứng.
+ Khi tệp được đóng, File Descriptor sẽ được giải phóng để sử dụng lại.

Ví dụ:
	# Mở và Đóng File Descriptor
	Mở tệp: Khi một chương trình gọi một hàm như open() hoặc fopen(), File Descriptor sẽ được tạo.
	Đóng tệp: Khi tệp được đóng bằng close(), File Descriptor sẽ bị giải phóng.
	Ví dụ:
		#include <fcntl.h>
		#include <unistd.h>

		int fd = open("file.txt", O_RDONLY);  // Mở tệp chỉ đọc
		if (fd == -1) 
		{
			perror("Error opening file");
		}
		close(fd);  // Đóng tệp
	# Chuyển Hướng File Descriptor trong Shell
	Ví dụ:
	" echo "Hello, Linux!" > output.txt "  : stdout (FD=1) chuyển hướng vào tệp

	# Kiểm Tra File Descriptor
	Kiểm tra các File Descriptor mở của một tiến trình bằng cách xem thư mục "/proc/<PID>/fd/"
	Ví dụ:
	" ls -l /proc/$$/fd " : $$ là một biến đặc biệt trong shell, đại diện cho PID của shell hiện tại đang chạy lệnh, bạn chạy lệnh này bạn liệt kê tất cả các File Descriptors của Shell hiện tại

	


