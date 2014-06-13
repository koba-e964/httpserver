require 'socket'
require 'date'

def http_header
  # Sun, 06 Nov 1994 08:49:37 GMT
  date_str = Time.now.getgm.strftime('%a, %d %b %Y %T GMT')
  p ["date:", date_str]
  <<-EOF
HTTP/1.1 200 OK
Content-Type: text/html; charset=UTF-8
Server: rserve.rb
Connection: close
Date: #{date_str}

EOF
end

def fail_header
  "HTTP/1.1 404 Not Found\r\n\r\n404 File Not Found\r\n\r\n"
end

def bad_request
  "HTTP/1.1 400 Bad Request\r\n\r\n400 Bad Request\r\n\r\n"
end

def get_content(name)
  puts "filename=#{name}"
  if (File.file? name) == false
    puts "(>_<) File does not exist"
    return fail_header
  end
  ss = File.open(name) { |f|
    f.read
  }
  http_header + ss + "\r\n\r\n"
end

def serve(docroot, sock)
  p sock.peeraddr
  filename = nil
  # the first line: [method] [request uri] [http-version]
  if !(line = sock.gets)
    puts "empty line (>_<)"
    sock.close
    return
  end
  p line
  method, uri, httpver = line.split
  if method == "GET"
    filename = uri
    if httpver != "HTTP/1.1"
      puts "WARNING!!! Not HTTP/1.1 (>_<)"
    end
  else
    sock.write bad_request
    sock.close
    return
  end
  while buf = sock.gets
    if buf.chomp == ""
      break
    end
    words = buf.split
    p words
  end
  if !filename
    return
  end
  content = get_content (docroot + filename)
  sock.write content
  sock.close
  puts "closing connection..."
end

port = if ARGV[0].nil? then 54321 else ARGV[0].to_i(10) end

docroot = ARGV[1] || "."

server = TCPServer.open port

while true
  if false
    Thread.start (server.accept) { |sock|
      serve(docroot, sock)
    }
  else
    serve(docroot, server.accept)
  end
end

server.close

