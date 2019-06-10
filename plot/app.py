# -*- coding: utf-8 -*-
import dash
import sys
import dash_core_components as dcc
import dash_html_components as html
import plotly.graph_objs as go
import logging
import os
import click
import glob
from file_loader import load
from threading import Lock

logger = logging.getLogger()
handler = logging.StreamHandler(sys.stdout)
formatter = logging.Formatter(
    '%(asctime)s %(levelname)-8s %(message)s')
handler.setFormatter(formatter)
logger.addHandler(handler)
logger.setLevel(logging.DEBUG)

metric_names = ['min', 'max', 'max_ind', 'average', 'median', 'variance', 'variance_bounded']

runtime = {}
runtime_lock = Lock()


def make_slider(param_name, slider_id):
    idx = '{}_num'.format(param_name)

    if param_name == 'metric':
        marks = {str(x): metric_names[x] for x in runtime['df'][idx].unique()}
    else:
        marks = {str(x): str(x) for x in runtime['df'][idx].unique()}

    return dcc.Slider(
        id='{}-slider-{}'.format(param_name, slider_id),
        min=runtime['df'][idx].min(),
        max=runtime['df'][idx].max(),
        value=runtime['df'][idx].min(),
        marks=marks
    )


def make_centered_p(text):
    return html.P(text, style={'text-align': 'center', 'font-style': 'bold'})


def update_file(selected_file):
    logging.debug('selected_file = %s, runtime_file = %s', selected_file, runtime['file'])
    if int(selected_file) == runtime['file']:
        return

    runtime['file'] = int(selected_file)
    logger.debug('Loading %s', os.path.expanduser(os.path.join(runtime['dir_path'], runtime['files'][runtime['file']])))
    runtime['df'] = load(os.path.expanduser(os.path.join(runtime['dir_path'], runtime['files'][runtime['file']])))


external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']


@click.command('run')
@click.argument('dir_path', type=str)
def run(dir_path):
    runtime['dir_path'] = os.path.expanduser(dir_path)

    if not os.path.exists(runtime['dir_path']):
        logger.error('Directory %s does not exist, exiting', runtime['dir_path'])
        return

    logger.debug(glob.glob(os.path.expanduser(runtime['dir_path'] + "/*.processed")))
    runtime['files'] = list(map(os.path.basename, glob.glob(os.path.expanduser(runtime['dir_path'] + "/*.processed"))))
    if len(runtime['files']) == 0:
        logger.error('List of .processed files is empty, exiting')
        return

    logger.debug(runtime['files'])

    runtime['file'] = None
    with runtime_lock:
        update_file(0)

    app = dash.Dash(__name__, external_stylesheets=external_stylesheets)

    app.layout = html.Div([
        html.H1('PRAO Data Viewer'),
        html.I('That’s one small step for a man, but one giant leap for mankind. (Neil Armstrong)'),
        html.Br(),
        html.Br(),

        dcc.Dropdown(
            options=[{'label': runtime['files'][i], 'value': str(i)} for i in range(len(runtime['files']))],
            value=str(runtime['file']),
            id='file-changer'
        ),

        html.Br(),

        html.H2('Rays by Band & Metric'),
        dcc.Graph(id='graph-band-metric'),
        make_slider('band', 3),
        html.Br(),
        make_centered_p('band'),
        make_slider('metric', 3),
        html.Br(),
        make_centered_p('metric'),

        html.H2('Bands by Ray & Metric'),
        dcc.Graph(id='graph-ray-metric'),
        make_slider('ray', 2),
        html.Br(),
        make_centered_p('ray'),
        make_slider('metric', 2),
        html.Br(),
        make_centered_p('metric'),

        html.H2('Metrics by Band & Ray'),
        dcc.Graph(id='graph-band-ray'),
        make_slider('band', 1),
        html.Br(),
        make_centered_p('band'),
        make_slider('ray', 1),
        html.Br(),
        make_centered_p('ray'),

    ], style={'margin-left': '50px', 'margin-right': '50px'})

    @app.callback(
        dash.dependencies.Output('graph-band-ray', 'figure'),
        [dash.dependencies.Input('ray-slider-1', 'value'),
         dash.dependencies.Input('band-slider-1', 'value'),
         dash.dependencies.Input('file-changer', 'value'),
         ])
    def update_figure_1(selected_ray, selected_band, selected_file):
        logger.debug('enter update_figure_1(%s, %s, %s)', selected_ray, selected_band, selected_file)
        if int(selected_file) != runtime['file']:
            with runtime_lock:
                update_file(selected_file)

        filtered_df = runtime['df'][(runtime['df']['ray_num'] == selected_ray) & (runtime['df']['band_num'] == selected_band)]
        traces = []
        for i in filtered_df.metric_num.unique():
            df_by_metric = filtered_df[filtered_df['metric_num'] == i]
            traces.append(go.Scatter(
                x=df_by_metric['ts'],
                y=df_by_metric['value'],
                x0=0,
                y0=0,
                mode='lines',
                opacity=0.7,
                marker={
                    'size': 15,
                    'line': {'width': 0.5, 'color': 'white'}
                },
                name=metric_names[i],
            ))

        logger.debug('leave update_figure_1(%s, %s, %s)', selected_ray, selected_band, selected_file)
        return {
            'data': traces,
            'layout': go.Layout(
                xaxis={'range': [0, 360]},
                yaxis={'range': [0, 1000]},
                margin={'l': 40, 'b': 40, 't': 10, 'r': 10},
                legend={'x': 0, 'y': 1},
                hovermode='closest'
            )
        }

    @app.callback(
        dash.dependencies.Output('graph-ray-metric', 'figure'),
        [dash.dependencies.Input('ray-slider-2', 'value'),
         dash.dependencies.Input('metric-slider-2', 'value'),
         dash.dependencies.Input('file-changer', 'value'),
         ])
    def update_figure_2(selected_ray, selected_metric, selected_file):
        logger.debug('enter update_figure_2(%s, %s, %s)', selected_ray, selected_metric, selected_file)
        if int(selected_file) != runtime['file']:
            with runtime_lock:
                update_file(selected_file)

        filtered_df = runtime['df'][(runtime['df']['ray_num'] == selected_ray) & (runtime['df']['metric_num'] == selected_metric)]
        traces = []
        for i in filtered_df.band_num.unique():
            df_by_metric = filtered_df[filtered_df['band_num'] == i]
            traces.append(go.Scatter(
                x=df_by_metric['ts'],
                y=df_by_metric['value'],
                x0=0,
                y0=0,
                mode='lines',
                opacity=0.7,
                marker={
                    'size': 15,
                    'line': {'width': 0.5, 'color': 'white'}
                },
                name=str(i)))

        logger.debug('leave update_figure_2(%s, %s, %s)', selected_ray, selected_metric, selected_file)
        return {
            'data': traces,
            'layout': go.Layout(
                xaxis={'range': [0, 360]},  # TODO: 360 считать по df
                yaxis={'range': [0, 1000]},
                margin={'l': 40, 'b': 40, 't': 10, 'r': 10},
                legend={'x': 0, 'y': 1},
                hovermode='closest'
            )
        }

    @app.callback(
        dash.dependencies.Output('graph-band-metric', 'figure'),
        [dash.dependencies.Input('band-slider-3', 'value'),
         dash.dependencies.Input('metric-slider-3', 'value'),
         dash.dependencies.Input('file-changer', 'value'),
         ])
    def update_figure_3(selected_band, selected_metric, selected_file):
        logger.debug('enter update_figure_3(%s, %s, %s)', selected_band, selected_metric, selected_file)
        if int(selected_file) != runtime['file']:
            with runtime_lock:
                update_file(selected_file)

        filtered_df = runtime['df'][(runtime['df']['band_num'] == selected_band) & (runtime['df']['metric_num'] == selected_metric)]
        traces = []
        for i in filtered_df.ray_num.unique():
            df_by_metric = filtered_df[filtered_df['ray_num'] == i]
            traces.append(go.Scatter(
                x=df_by_metric['ts'],
                y=df_by_metric['value'],
                x0=0,
                y0=0,
                mode='lines',
                opacity=0.7,
                marker={
                    'size': 15,
                    'line': {'width': 0.5, 'color': 'white'}
                },
                name=str(i),
            ))

        logging.debug('leave update_figure_3(%s, %s, %s)', selected_band, selected_metric, selected_file)
        return {
            'data': traces,
            'layout': go.Layout(
                xaxis={'range': [0, 360]},
                yaxis={'range': [0, 1000]},
                margin={'l': 40, 'b': 40, 't': 10, 'r': 10},
                legend={'x': 0, 'y': 1},
                hovermode='closest'
            )
        }

    app.run_server(debug=False)


if __name__ == '__main__':
    run()
