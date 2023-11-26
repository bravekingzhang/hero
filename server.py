import http.server
import socketserver


def serve():
    Handler = http.server.SimpleHTTPRequestHandler
    httpd = socketserver.TCPServer(("", 8000), Handler)
    print("serving at port", 8000)
    httpd.serve_forever()
