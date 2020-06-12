# -*- coding: utf-8 -*-
import array
import itertools
import os
import time
import logging
import pandas as pd
from struct import unpack
import json

# logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger()


def join_lists(list_lists):
    return list(itertools.chain.from_iterable(list_lists))


class file_metrics:
    header = {}
    df = pd.DataFrame()

    def __init__(self, file_path):
        self.read_from_file(file_path)

    def read_from_file(self, file_path):
        with open(file_path, 'rb') as f:
            header_length = unpack('I', f.read(4))[0]
            logger.debug('header_length = %d', header_length)

            self.header = json.loads(f.read(header_length))
            self.header['nrays'] = len(self.header['source_file']['modulus']) * 8
            logging.debug(str(self.header))

            file_length = os.path.getsize(file_path) - header_length - 4
            logger.debug('File size w/o header is {} B'.format(file_length))

            data = array.array('f')
            data.fromfile(f, file_length // 4)

            npoints = self.header['npoints_zipped']
            nmetrics = len(self.header['metrics'])
            nrays = self.header['nrays']
            nbands = self.header['source_file']['nbands'] + 1

            if "N1" in self.header['source_file']['source_file_start']:
                self.header['isnorth'] = True
            elif "N2" in self.header['source_file']['source_file_start']:
                self.header['isnorth'] = False


            time_started = time.time()
            struct = {
                'metric_num': [i for i in range(nmetrics)] * (npoints * nrays * nbands),
                'band_num': join_lists([[i] * nmetrics for i in range(nbands)]) * (npoints * nrays),
                'ray_num': join_lists([[i] * nbands * nmetrics for i in range(nrays)]) * npoints,
                'ts': join_lists([[i] * (nmetrics * nrays * nbands) for i in range(npoints)]),

                'value': data
            }

            for key in struct:
                logging.debug('%s %d', key, len(struct[key]))

            self.df = pd.DataFrame(struct)
            time_finished = time.time()

            logger.debug('{0:.2f}s elapsed'.format(time_finished - time_started))

