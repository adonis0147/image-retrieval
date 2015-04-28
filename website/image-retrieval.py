#!/usr/bin/env python
# coding: utf-8

import tornado.web
import tornado.ioloop
import tornado.httpserver
import os.path
import struct
import json
import random
import socket
import struct
from pymongo import MongoClient
from bson import json_util
from pprint import pprint

class Application(tornado.web.Application):
    def __init__(self):
        handlers = [
            (r'/', HomeHandler),
            (r'/query/(\d+)', QueryHandler),
        ]

        settings = dict(
            static_path = os.path.join(os.path.dirname(__file__), 'static'),
            template_path = os.path.join(os.path.dirname(__file__), 'templates'),
            xsrf_cookies = True,
            cookie_secret = 'eb1970472b5e5649c4eee2ee931e1baa',
        )

        tornado.web.Application.__init__(self, handlers, **settings)

        self.test_data = LoadTestData('../data/features_jd_test2.dat', 128)

        client = MongoClient('mongodb://localhost:27017/')
        self.db = client['jd']


class BaseHandler(tornado.web.RequestHandler):
    @property
    def test_data(self):
        return self.application.test_data

    @property
    def db(self):
        return self.application.db


class HomeHandler(BaseHandler):
    def get(self):
        items = self.GetRandomItems(10)
        self.render('index.html', home_title='Clothing Image Retrieval',
                    items=items)


    def GetRandomItems(self, k):
        test_size = len(self.test_data)
        selected_ids = random.sample(xrange(test_size), k)
        db_items = self.db.jd_info_test.find({'id': {'$in': selected_ids}}, {'_id': 0})
        return list(db_items)


class QueryHandler(BaseHandler):
    def get(self, query_id):
        query_id = int(query_id)
        result_ids = self.query(query_id, 10, 100, 128)
        mongo_items = self.db.jd_info_train.find({'id': {'$in': result_ids}}, {'_id': 0})
        items = []
        for mongo_item in mongo_items:
            item_id = mongo_item[u'id']
            path = mongo_item[u'path'].lower()
            item = {'id': item_id, 'path': path}
            jd_item_id = int(path.split('-')[-1][:-4])
            item['url'] = 'http://item.jd.com/%d.html' % jd_item_id
            items.append(item)

        self.render('results.html', home_title='Clothing Image Retrieval',
                    items=items)


    def query(self, query_id, num_neighbors, search_radius, dim):
        address = ('localhost', 5000)
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(address)

        query = self.test_data[query_id]
        request_format = '<III%s' % ('B' * dim)
        request_msg = struct.pack(request_format, num_neighbors, search_radius,
                                  dim, *query)
        s.send(request_msg)

        response_msg = s.recv(1024)
        num_results = int(struct.unpack('<I', response_msg[:4])[0])
        response_format = '<%s' % ('I' * num_results)
        return struct.unpack(response_format, response_msg[4:])


def LoadTestData(file_name, dim):
    test_data = []
    with open(file_name, 'rb') as f:
        test_size = struct.unpack('<I', f.read(4))[0]
        binary_format = '<' + 'B' * dim
        for i in xrange(test_size):
            test_data.append(struct.unpack(binary_format, f.read(dim)))
    return test_data


def Main():
    http_server = tornado.httpserver.HTTPServer(Application())
    http_server.listen(8888)
    tornado.ioloop.IOLoop.instance().start()


if __name__ == '__main__':
    Main()
