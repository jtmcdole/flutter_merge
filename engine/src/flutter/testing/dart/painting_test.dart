// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:typed_data';
import 'dart:ui';

import 'package:test/test.dart';
import 'package:vector_math/vector_math_64.dart';

typedef CanvasCallback = void Function(Canvas canvas);

void main() {
  test('Vertices checks', () {
    try {
      Vertices(
        VertexMode.triangles,
        const <Offset>[Offset.zero, Offset.zero, Offset.zero],
        indices: Uint16List.fromList(const <int>[0, 2, 5]),
      );
      throw 'Vertices did not throw the expected error.';
    } on ArgumentError catch (e) {
      expect('$e', 'Invalid argument(s): "indices" values must be valid indices in the positions list (i.e. numbers in the range 0..2), but indices[2] is 5, which is too big.');
    }
    Vertices( // This one does not throw.
      VertexMode.triangles,
      const <Offset>[Offset.zero],
    ).dispose();
    Vertices( // This one should not throw.
      VertexMode.triangles,
      const <Offset>[Offset.zero, Offset.zero, Offset.zero],
      indices: Uint16List.fromList(const <int>[0, 2, 1, 2, 0, 1, 2, 0]), // Uint16List implements List<int> so this is ok.
    ).dispose();
  });

  test('Vertices.raw checks', () {
    expect(() {
      Vertices.raw(
        VertexMode.triangles,
        Float32List.fromList(const <double>[0.0]),
      );
    }, throwsA(isA<ArgumentError>().having((ArgumentError e) => '$e', 'message', 'Invalid argument(s): "positions" must have an even number of entries (each coordinate is an x,y pair).')));

    Object? indicesError;
    try {
      Vertices.raw(
        VertexMode.triangles,
        Float32List.fromList(const <double>[0.0, 0.0, 0.0, 0.0, 0.0, 0.0]),
        indices: Uint16List.fromList(const <int>[0, 2, 5]),
      );
      throw 'Vertices.raw did not throw the expected error.';
    } on ArgumentError catch (e) {
      indicesError = e;
    }
    expect('$indicesError', 'Invalid argument(s): "indices" values must be valid indices in the positions list (i.e. numbers in the range 0..2), but indices[2] is 5, which is too big.');

    Vertices.raw( // This one does not throw.
      VertexMode.triangles,
      Float32List.fromList(const <double>[0.0, 0.0]),
    ).dispose();
    Vertices.raw( // This one should not throw.
      VertexMode.triangles,
      Float32List.fromList(const <double>[0.0, 0.0, 0.0, 0.0, 0.0, 0.0]),
      indices: Uint16List.fromList(const <int>[0, 2, 1, 2, 0, 1, 2, 0]),
    ).dispose();
  });

  test('BackdropFilter with multiple clips', () async {
    // Regression test for https://github.com/flutter/flutter/issues/144211
    Picture makePicture(CanvasCallback callback) {
      final PictureRecorder recorder = PictureRecorder();
      final Canvas canvas = Canvas(recorder);
      callback(canvas);
      return recorder.endRecording();
    }
    final SceneBuilder sceneBuilder = SceneBuilder();

    final Picture redClippedPicture = makePicture((Canvas canvas) {
      canvas.drawPaint(Paint()..color = const Color(0xFFFFFFFF));
      canvas.clipRect(const Rect.fromLTRB(10, 10, 200, 200));
      canvas.clipRect(const Rect.fromLTRB(11, 10, 300, 200));
      canvas.drawPaint(Paint()..color = const Color(0xFFFF0000));
    });
    sceneBuilder.addPicture(Offset.zero, redClippedPicture);

    final Float64List matrix = Float64List(16);
    sceneBuilder.pushBackdropFilter(ImageFilter.matrix(matrix));

    final Picture whitePicture = makePicture((Canvas canvas) {
      canvas.drawPaint(Paint()..color = const Color(0xFFFFFFFF));
    });
    sceneBuilder.addPicture(Offset.zero, whitePicture);

    final Scene scene = sceneBuilder.build();
    final Image image = scene.toImageSync(20, 20);

    final ByteData data = (await image.toByteData())!;
    expect(data.buffer.asUint32List().length, 20 * 20);
    // If clipping went wrong as in the linked issue, there will be red pixels.
    for (final int color in  data.buffer.asUint32List()) {
      expect(color, 0xFFFFFFFF);
    }

    scene.dispose();
    image.dispose();
    whitePicture.dispose();
    redClippedPicture.dispose();
  });

  test('ImageFilter.matrix defaults to FilterQuality.medium', () {
    final Float64List data = Matrix4.identity().storage;
    expect(
      ImageFilter.matrix(data).toString(),
      'ImageFilter.matrix($data, FilterQuality.medium)',
    );
  });

  test('Picture.toImage generates mip maps', () async {
    // Draw a grid of red and blue squares. When averaged together via
    // mip maps, this should result in a purplish color. If there are no
    // mip maps, the original red and blue colors will be preserved regardless
    // of scale or number of pixels.
    late final Image image;
    {
      final PictureRecorder recorder = PictureRecorder();
      final Canvas canvas = Canvas(recorder);
      for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 20; j++) {
          final Color color = (i + j).isEven ? const Color(0xFFFF0000) : const Color(0xFF0000FF);
          canvas.drawRect(Rect.fromLTWH(i * 5, j * 5, 5, 5), Paint()..color = color);
        }
      }
      final Picture picture = recorder.endRecording();
      image = await picture.toImage(100, 100);
    }

    final PictureRecorder recorder = PictureRecorder();
    final Canvas canvas = Canvas(recorder);
    canvas.save();
    canvas.scale(0.25, 0.25);
    canvas.drawImage(image, Offset.zero, Paint()..filterQuality = FilterQuality.medium);
    canvas.restore();

    final Picture picture = recorder.endRecording();
    final Image resultImage = await picture.toImage(10, 10);
    final ByteData data = (await resultImage.toByteData())!;

    final Int32List colors = data.buffer.asInt32List();
    for (int i = 0; i < colors.length; i++) {
      expect(colors[i], isNot(const Color(0xFFFF0000)));
      expect(colors[i], isNot(const Color(0xFF0000FF)));
    }
  });
}
