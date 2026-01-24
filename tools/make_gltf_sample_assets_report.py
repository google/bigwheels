"""Renders all glTF-Sample-Assets using gltf_scene_viewer and produces a report."""

import argparse
import json
import os
import pathlib
import shutil
import subprocess
import xml.etree.ElementTree as ET


def _make_report(input_path: pathlib.Path,
                 model_index_path: pathlib.Path,
                 output_path: pathlib.Path):
  """Generates an HTML website with a table of test results.

  This does several things:

  1. Generates index.html containing the report text.
  2. Copies and converts artifacts into `output_path` so that it's shareable.

  Arguments:
    input_path: Location of test results.
    model_index_path: Path to glTF-Sample-Assets model-index.json.
    output_path: Destination of the HTML report and associated artifacts.
  """

  model_index_dir = model_index_path.absolute().parent

  with model_index_path.open('r', encoding="utf-8") as fd:
    model_index = json.load(fd)

  with (input_path / 'meta.json').open('r') as meta_file:
    meta = json.load(meta_file)

  html = ET.Element('html')

  # Alternate row color to make table easier to parse
  head = ET.SubElement(html, 'head')
  ET.SubElement(head, 'style').text = (
      'tbody tr:nth-child(odd) { background-color: #eee; }')

  body = ET.SubElement(html, 'body')
  ET.SubElement(body, 'p').text = f'Time: {meta["datetime"]}'
  ET.SubElement(body, 'p').text = (
      f'BigWheels Commit SHA: {meta["bigwheels_commit_sha"]}')
  ET.SubElement(body, 'p').text = (
      f'gltf-Sample-Assets Commit SHA: {meta["glTF-Sample-Assets_commit_sha"]}')
  ET.SubElement(body, 'p').text = f'Host: {meta["host"]}'

  table = ET.SubElement(body, 'table')

  thead_tr = ET.SubElement(ET.SubElement(table, 'thead'), 'tr')
  ET.SubElement(thead_tr, 'th').text = 'Label'
  ET.SubElement(thead_tr, 'th').text = 'glTF-Sample-Assets Screenshot'
  ET.SubElement(thead_tr, 'th').text = 'BigWheels Screenshot'
  ET.SubElement(thead_tr, 'th').text = 'Logs'

  tbody = ET.SubElement(table, 'tbody')
  for model in model_index:
    label = model['label']  # human readable
    name = model['name']  # path in glTF-Sample-Assets repo
    screenshot = model['screenshot']
    variants = model['variants']

    for variant in variants:
      test_name = f'{name}-{variant}'
      test_input_path = input_path / test_name

      # Either:
      # 1. The scene wasn't tested (partial results), or
      # 2. There's a mismatch between the model-index.json used to run
      #    the test and make the report.
      if not test_input_path.exists():
        continue

      test_output_path = output_path / test_name
      os.mkdir(test_output_path)

      tr = ET.SubElement(tbody, 'tr')

      # Label
      ET.SubElement(tr, 'td').text = f'{label} ({variant})'

      # glTF-Sample-Assets Screenshot
      # (Use relative paths in <img> and <a> so we can easily share)
      sample_ext = os.path.splitext(screenshot)[1]
      shutil.copy2(model_index_dir / name / screenshot,
                   test_output_path / f'expected{sample_ext}')
      ET.SubElement(
          ET.SubElement(tr, 'td'),
          'img', src=f'{test_name}/expected{sample_ext}', width='640px')

      # BigWheels Screenshot
      # (There won't be a screenshot if the scene fails to load)
      if (test_input_path / 'actual.ppm').exists():
        # Convert PPM -> PNG to support more browsers.
        # Use an external program since Python lacks an image standard lib.
        subprocess.run(['convert',
                        str(test_input_path / 'actual.ppm'),
                        str(test_output_path / 'actual.png')], check=True)
        ET.SubElement(
            ET.SubElement(tr, 'td'),
            'img', src=f'{test_name}/actual.png', width='640px')
      else:
        ET.SubElement(tr, 'td').text = 'None!'

      # Logs
      logs_td = ET.SubElement(tr, 'td')
      shutil.copy2(test_input_path / 'stdout.log',
                   test_output_path / 'stdout.log')
      ET.SubElement(
          ET.SubElement(logs_td, 'p'),
          'a', href=f'{test_name}/stdout.log').text = 'stdout.log'
      shutil.copy2(test_input_path / 'stderr.log',
                   test_output_path / 'stderr.log')
      ET.SubElement(
          ET.SubElement(logs_td, 'p'),
          'a', href=f'{test_name}/stderr.log').text = 'stderr.log'
      shutil.copy2(test_input_path / 'ppx.log', test_output_path / 'ppx.log')
      ET.SubElement(
          ET.SubElement(logs_td, 'p'),
          'a', href=f'{test_name}/ppx.log').text = 'ppx.log'

  ET.ElementTree(element=html).write(output_path / 'index.html', method='html')


def main():
  parser = argparse.ArgumentParser(
      description=('Creates an HTML report given the output of ' +
                   'test_gltf_sample_assets.py'))
  parser.add_argument('--input', type=pathlib.Path, required=True,
                      help='Directory created by test_gltf_sample_assets.py.')
  parser.add_argument('--model-index', type=pathlib.Path, required=True,
                      help='Path to glTF-Sample-Asssets model-index.json.')
  parser.add_argument('--output', type=pathlib.Path, required=True,
                      help='Directory to store the generated report.')
  args = parser.parse_args()

  os.mkdir(args.output)
  _make_report(args.input, args.model_index, args.output)


if __name__ == '__main__':
  main()
