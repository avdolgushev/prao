# -*- coding: utf-8 -*-
import array
import itertools
import os
import time
import logging
import pandas as pd
from struct import unpack
import json

#logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger()


def __join(list_lists):
    return list(itertools.chain.from_iterable(list_lists))


def load(path):

    as_float_array = array.array('f')

    with open(path, 'rb') as fp:
        header_length = unpack('I', fp.read(4))[0]
        logger.debug('header_length = %d', header_length)
        header = json.loads(fp.read(header_length))

        logging.debug(str(header))
        file_length = os.path.getsize(path) - header_length - 4
        logger.debug('File size w/o header is {} B'.format(file_length))
        as_float_array.fromfile(fp, file_length // 4)

    time_started = time.time()



    npoints = header['npoints_zipped']
    nmetrics = len(header['metrics'])
    nrays = len(header['source_file']['modulus']) * 8
    nbands = header['source_file']['nbands'] + 1

    nfloats = len(as_float_array)

    struct = {
		'metric_num': [i for i in range(nmetrics)] * (npoints * nrays * nmetrics),
		'band_num': __join([[i] * nmetrics for i in range(nbands)]) * (npoints * nrays),
		'ray_num': __join([[i] * nbands * nmetrics for i in range(nrays)]) * npoints,
		'ts': __join([[i] * (nmetrics * nrays * nbands) for i in range(npoints)]),
		
        'value': as_float_array
    }

    for key in struct:
        logging.debug('%s %d', key, len(struct[key]))

    df = pd.DataFrame(struct)
    time_finished = time.time()

    logger.debug('{0:.2f}s elapsed'.format(time_finished - time_started))

    return df