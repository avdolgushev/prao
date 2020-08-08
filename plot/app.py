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
from file_loader import file_metrics
from threading import Lock

logger = logging.getLogger()
handler = logging.StreamHandler(sys.stdout)
formatter = logging.Formatter(
    '%(asctime)s %(levelname)-8s %(message)s')
handler.setFormatter(formatter)
logger.addHandler(handler)
logger.setLevel(logging.DEBUG)

runtime = {}
runtime_lock = Lock()


def make_slider(param_name, slider_id):
    idx = '{}_num'.format(param_name)
    logging.info('idx = %s' % idx)

    if param_name == 'metric':
        metrics = list(runtime['metrics_obj'].header['metrics'])
        marks = {str(x): metrics[x] for x in range(len(metrics))}
    elif param_name == 'ray':
        if runtime['metrics_obj'].header['isnorth']:
            marks = {str(x): "{:.2f}".format(rays_gradient_north[str(x)])
                     for x in range(runtime['metrics_obj'].header['nrays'])}
        elif not runtime['metrics_obj'].header['isnorth']:
            marks = {str(x): "{:.2f}".format(rays_gradient_south[str(x)])
                     for x in range(runtime['metrics_obj'].header['nrays'])}
    elif param_name == 'band':
        fbands = runtime['metrics_obj'].header['source_file']['fbands']
        marks = {str(i): "{:.2f}MHz".format(fbands[i]) for i in range(len(fbands))}
        marks[str(len(fbands))] = "General"

    return dcc.Slider(
        id='{}-slider-{}'.format(param_name, slider_id),
        min=runtime['metrics_obj'].df[idx].min(),
        max=runtime['metrics_obj'].df[idx].max(),
        value=runtime['metrics_obj'].df[idx].min(),
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
    runtime['metrics_obj'] = file_metrics(
        os.path.expanduser(os.path.join(runtime['dir_path'], runtime['files'][runtime['file']])))


external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']


def seconds_to_time(seconds):
    hours = seconds // 3600
    seconds -= 3600 * hours
    minutes = seconds // 60
    seconds -= 60 * minutes
    return "{}:{}:{}".format(hours, minutes, seconds)


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
    delta_slider = dcc.Slider(
        id='delta-slider',
        min=5,
        max=500,
        value=10
    )

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
        html.Br(),
        make_slider('band', 1),
        html.Br(),
        make_centered_p('band'),
        make_slider('metric', 1),
        html.Br(),
        make_centered_p('metric'),

        html.H2('Rays by Band & Metric Beta'),
        dcc.Graph(id='graph-band-metric-beta'),
        html.Br(),
        make_slider('band', 4),
        html.Br(),
        make_centered_p('band'),
        make_slider('metric', 4),
        html.Br(),
        make_centered_p('metric'),
        html.Br(),
        delta_slider,
        make_centered_p('delta'),

        html.H2('Bands by Ray & Metric'),
        dcc.Graph(id='graph-ray-metric'),
        html.Br(),
        make_slider('ray', 2),
        html.Br(),
        make_centered_p('ray'),
        make_slider('metric', 2),
        html.Br(),
        make_centered_p('metric'),

        html.H2('Metrics by Band & Ray'),
        dcc.Graph(id='graph-band-ray'),
        html.Br(),
        make_slider('band', 3),
        html.Br(),
        make_centered_p('band'),
        make_slider('ray', 3),
        html.Br(),
        make_centered_p('ray'),

    ], style={'margin-left': '50px', 'margin-right': '50px'})

    @app.callback(
        dash.dependencies.Output('graph-band-metric', 'figure'),
        [dash.dependencies.Input('band-slider-1', 'value'),
         dash.dependencies.Input('metric-slider-1', 'value'),
         dash.dependencies.Input('file-changer', 'value'),
         ])
    def update_figure_1(selected_band, selected_metric, selected_file):
        logger.debug('enter update_figure_1(%s, %s, %s)', selected_band, selected_metric, selected_file)
        if int(selected_file) != runtime['file']:
            with runtime_lock:
                update_file(selected_file)

        df = runtime['metrics_obj'].df
        filtered_df = df[(df['band_num'] == selected_band) & (df['metric_num'] == selected_metric)]
        logging.debug("filtered df is %s\n", filtered_df.head(10))
        traces = []
        x_ = list(range(int(runtime['metrics_obj'].header['star_start']), int(
            runtime['metrics_obj'].header['star_start'] + runtime['metrics_obj'].header[
                'fileDuration_in_star_seconds']), int(runtime['metrics_obj'].header['zipped_point_tresolution'])))
        x_ = [seconds_to_time(i) for i in x_]
        if runtime['metrics_obj'].header['isnorth']:
            filtered_df['ray_num'] = filtered_df['ray_num'].apply(lambda ray: rays_gradient_north[str(ray)])
        elif not runtime['metrics_obj'].header['isnorth']:
            filtered_df['ray_num'] = filtered_df['ray_num'].apply(lambda ray: rays_gradient_south[str(ray)])
        for ray_degree in filtered_df.ray_num.unique():
            df_by_metric = filtered_df[filtered_df['ray_num'] == ray_degree]
            traces.append(go.Scatter(
                x=x_,
                y=df_by_metric['value'],
                x0=0,
                y0=0,
                mode='lines+text',
                text=[ray_degree],
                opacity=0.7,
                marker={
                    'size': 15,
                    'line': {'width': 0.5, 'color': 'white'}
                },
                name=str(ray_degree),
            ))

        logging.debug('leave update_figure_1(%s, %s, %s)', selected_band, selected_metric, selected_file)
        return {
            'data': traces,
            'layout': go.Layout(
                xaxis={'range': [x_[0], x_[-1]]},
                yaxis={'range': [0, 2500], 'autorange': True},
                margin={'l': 40, 'b': 40, 't': 10, 'r': 10},
                hovermode='closest',
                legend=dict(
                    yanchor="top",
                    xanchor="right",
                    orientation="h",
                    x=1,
                    y=1.3
                )
            )
        }

    @app.callback(
        dash.dependencies.Output('graph-band-metric-beta', 'figure'),
        [dash.dependencies.Input('band-slider-4', 'value'),
         dash.dependencies.Input('metric-slider-4', 'value'),
         dash.dependencies.Input('file-changer', 'value'),
         dash.dependencies.Input('delta-slider', 'value')
         ])
    def update_figure_4(selected_band, selected_metric, selected_file, delta):
        logger.debug('enter update_figure_4(%s, %s, %s)', selected_band, selected_metric, selected_file)
        if int(selected_file) != runtime['file']:
            with runtime_lock:
                update_file(selected_file)

        df = runtime['metrics_obj'].df
        filtered_df = df[(df['band_num'] == selected_band) & (df['metric_num'] == selected_metric)]
        logging.debug("filtered df is %s\n", filtered_df.head(10))
        traces = []
        x_ = list(range(int(runtime['metrics_obj'].header['star_start']), int(
            runtime['metrics_obj'].header['star_start'] + runtime['metrics_obj'].header[
                'fileDuration_in_star_seconds']), int(runtime['metrics_obj'].header['zipped_point_tresolution'])))
        x_ = [seconds_to_time(i) for i in x_]
        if runtime['metrics_obj'].header['isnorth']:
            filtered_df['ray_num'] = filtered_df['ray_num'].apply(lambda ray: rays_gradient_north[str(ray)])
        elif not runtime['metrics_obj'].header['isnorth']:
            filtered_df['ray_num'] = filtered_df['ray_num'].apply(lambda ray: rays_gradient_south[str(ray)])
        logging.debug("filtered df is %s", filtered_df)
        normalized_df = filtered_df.copy()
        multiplier = 0
        logging.debug("unique rays are %s", normalized_df.ray_num.unique())

        for ray_degree in normalized_df.ray_num.unique():
            minvalue = normalized_df[normalized_df['ray_num'] == ray_degree]['value'].min()
            normalized_df.loc[normalized_df['ray_num'] == ray_degree, 'value'] = (normalized_df['value'] - minvalue) \
                                                                                 + delta * multiplier
            df_by_metric = normalized_df[normalized_df['ray_num'] == ray_degree]
            traces.append(go.Scatter(
                x=x_,
                y=df_by_metric['value'],
                x0=0,
                y0=0,
                mode='lines+text',
                text=[ray_degree],
                opacity=0.7,
                marker={
                    'size': 10,
                    'line': {'width': 0.5, 'color': 'black'}
                },
                name=str(ray_degree),
            ))
            multiplier += 1

        logging.debug('leave update_figure_4(%s, %s, %s)', selected_band, selected_metric, selected_file)
        return {
            'data': traces,
            'layout': go.Layout(
                xaxis={'range': [x_[0], x_[-1]]},
                yaxis={'range': [0, 2500], 'visible': False, 'autorange': True},
                margin={'l': 10, 'b': 40, 't': 10, 'r': 10},
                legend={'x': 0, 'y': 0},
                hovermode='closest',
                height=800,
                width=1200,
                showlegend=False
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

        df = runtime['metrics_obj'].df
        filtered_df = df[(df['ray_num'] == selected_ray) & (df['metric_num'] == selected_metric)]
        traces = []
        x_ = list(range(int(runtime['metrics_obj'].header['star_start']), int(
            runtime['metrics_obj'].header['star_start'] + runtime['metrics_obj'].header[
                'fileDuration_in_star_seconds']), int(runtime['metrics_obj'].header['zipped_point_tresolution'])))
        x_ = [seconds_to_time(i) for i in x_]
        fbands = runtime['metrics_obj'].header['source_file']['fbands']
        logging.debug("fbands are %s", fbands)
        fbands = ["{:.2f}MHz".format(x) for x in fbands]
        fbands.append("General")
        filtered_df['band_num'] = filtered_df['band_num'].apply(lambda band_num: fbands[band_num])
        for i in filtered_df.band_num.unique():
            df_by_metric = filtered_df[filtered_df['band_num'] == i]
            traces.append(go.Scatter(
                x=x_,
                y=df_by_metric['value'],
                x0=0,
                y0=0,
                mode='lines+text',
                text=[i],
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
                xaxis={'range': [x_[0], x_[-1]]},
                yaxis={'range': [0, 2500], 'autorange': True},
                margin={'l': 40, 'b': 40, 't': 10, 'r': 10},
                legend=dict(
                    xanchor="right",
                    yanchor="top",
                    orientation="h",
                    x=1,
                    y=1.3
                ),
                hovermode='closest'
            )
        }

    @app.callback(
        dash.dependencies.Output('graph-band-ray', 'figure'),
        [dash.dependencies.Input('band-slider-3', 'value'),
         dash.dependencies.Input('ray-slider-3', 'value'),
         dash.dependencies.Input('file-changer', 'value'),
         ])
    def update_figure_3(selected_band, selected_ray, selected_file):
        logger.debug('enter update_figure_3(%s, %s, %s)', selected_ray, selected_band, selected_file)
        if int(selected_file) != runtime['file']:
            with runtime_lock:
                update_file(selected_file)

        df = runtime['metrics_obj'].df
        filtered_df = df[(df['ray_num'] == selected_ray) & (df['band_num'] == selected_band)]
        traces = []
        x_ = list(range(int(runtime['metrics_obj'].header['star_start']), int(
            runtime['metrics_obj'].header['star_start'] + runtime['metrics_obj'].header[
                'fileDuration_in_star_seconds']), int(runtime['metrics_obj'].header['zipped_point_tresolution'])))
        x_ = [seconds_to_time(i) for i in x_]
        for i in filtered_df.metric_num.unique():
            df_by_metric = filtered_df[filtered_df['metric_num'] == i]
            traces.append(go.Scatter(
                x=x_,
                y=df_by_metric['value'],
                x0=0,
                y0=0,
                mode='lines+text',
                text=[runtime['metrics_obj'].header['metrics'][i]],
                opacity=0.7,
                marker={
                    'size': 15,
                    'line': {'width': 0.5, 'color': 'white'}
                },
                name=runtime['metrics_obj'].header['metrics'][i],
            ))

        logger.debug('leave update_figure_3(%s, %s, %s)', selected_ray, selected_band, selected_file)
        return {
            'data': traces,
            'layout': go.Layout(
                xaxis={'range': [x_[0], x_[-1]]},
                yaxis={'range': [0, 2500], 'autorange': True},
                margin={'l': 40, 'b': 40, 't': 10, 'r': 10},
                legend=dict(
                    orientation="h",
                    xanchor="right",
                    yanchor="bottom",
                    x=1,
                    y=1.02
                ),
                hovermode='closest'
            )
        }

    app.run_server(debug=False)


"""
Склонения каждого луча в градусах
Inclination of every ray in degrees
(idk how to calculate it)
"""
rays_gradient_north = {'0': 42.13, '1': 41.72, '2': 41.31, '3': 40.89, '4': 40.47, '5': 40.06,
                       '6': 39.64, '7': 39.23, '8': 38.79, '9': 38.38, '10': 37.95, '11': 37.54,
                       '12': 37.11, '13': 37.69, '14': 36.26, '15': 35.85, '16': 35.40, '17': 34.97,
                       '18': 34.54, '19': 34.12, '20': 33.69, '21': 33.25, '22': 32.82, '23': 32.38,
                       '24': 31.94, '25': 31.5, '26': 31.06, '27': 30.61, '28': 30.17, '29': 29.73,
                       '30': 29.29, '31': 28.84, '32': 28.37, '33': 27.92, '34': 27.47, '35': 27.01,
                       '36': 26.56, '37': 26.1, '38': 25.64, '39': 25.18, '40': 24.7, '41': 24.23,
                       '42': 23.76, '43': 23.29, '44': 22.81, '45': 22.34, '46': 21.86, '47': 21.38}

rays_gradient_south = {'0': 20.8, '1': 20.4, '2': 19.9, '3': 19.4, '4': 18.9, '5': 18.4,
                       '6': 17.9, '7': 17.4, '8': 16.9, '9': 16.4, '10': 15.8, '11': 15.3,
                       '12': 14.8, '13': 14.3, '14': 13.7, '15': 13.2, '16': 12.6, '17': 12.1,
                       '18': 11.5, '19': 11.0, '20': 10.4, '21': 9.8, '22': 9.3, '23': 8.7,
                       '24': 8.1, '25': 7.5, '26': 6.9, '27': 6.3, '28': 5.7, '29': 5.0,
                       '30': 4.4, '31': 3.8, '32': 3.1, '33': 2.5, '34': 1.8, '35': 1.1,
                       '36': 0.4, '37': -0.3, '38': -1.0, '39': -1.7, '40': -2.5, '41': -3.2,
                       '42': -4.0, '43': -4.8, '44': -5.6, '45': -6.5, '46': -7.3, '47': -8.2}

if __name__ == '__main__':
    run()
