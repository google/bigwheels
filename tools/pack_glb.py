import argparse
import json
import logging
import os
from pathlib import Path
import sys
from typing import Any, BinaryIO
from typing_extensions import NotRequired, TypedDict
from urllib.parse import urlparse


def align_to_4(value: int) -> int:
    return (value + 3) & ~3


class GLTFError(Exception):
    """General exception class for GLTF and GLB errors"""


class GLTFBufferDescription(TypedDict):
    byteLength: int
    uri: NotRequired[str]


class GLTFBufferViewDescription(TypedDict):
    buffer: int
    byteLength: int
    byteOffset: int


class GLTFImageDescription(TypedDict):
    uri: NotRequired[str]
    bufferView: int
    mimeType: str


class GLTFDescription(TypedDict):
    buffers: list[GLTFBufferDescription]
    bufferViews: list[GLTFBufferViewDescription]
    images: list[GLTFImageDescription]


class GLB:
    """Holds the data necessary to create a GLB file."""

    def __init__(self, description: GLTFDescription, source_dir: Path) -> None:
        self.description = description
        self.source_dir = source_dir
        self.bin_length = 0
        self.source_files: list[Path] = []

    def pack(self) -> None:
        """Rewrite all references to external files as references to a packed buffer."""
        self._pack_buffers()
        self._pack_images()
        self.description["buffers"] = [{"byteLength": self.bin_length}]

        if self.bin_length > 2**32 - 1:
            logging.warning(
                "Packed internal buffer exceeds recommended max byte length of 2**32-1: %i",
                self.bin_length,
            )

    def _pack_buffers(self) -> None:
        # We are packing potentially multiple external binary buffer files into a single internal buffer
        # This maps the index of each external buffer to an offset in the internal buffer
        buffer_offsets: dict[int, int] = {}

        for index, buffer in enumerate(self.description["buffers"]):
            if "uri" in buffer:
                # this doesn't handle data uris; we can add support if necessary
                uri = urlparse(buffer["uri"])
                if uri.scheme != "" or uri.netloc != "":
                    raise GLTFError(f'support for uri not implemented: "{uri}"')
                # record the path of the locally stored external buffer to be packed
                self.source_files.append(Path(uri.path))
                buffer_offsets[index] = self.bin_length
            else:
                if index != 0:
                    raise GLTFError('Only the first buffer may omit the "uri" property')

            self.bin_length += buffer["byteLength"]

        # buffer views need to be rewritten to reference offsets inside the internal buffer
        for buffer_view in self.description["bufferViews"]:
            if buffer_view["buffer"] in buffer_offsets:
                if "byteOffset" not in buffer_view:
                    buffer_view["byteOffset"] = 0
                buffer_view["byteOffset"] += buffer_offsets[buffer_view["buffer"]]
                buffer_view["buffer"] = 0

    def _pack_images(self) -> None:
        # Rewrite images to reference buffer views inside the internal buffer
        for image in self.description["images"]:
            if "uri" in image:
                uri = urlparse(image["uri"])
                if uri.scheme != "" or uri.netloc != "":
                    raise GLTFError(f'support for uri not implemented: "{uri}"')
                self.source_files.append(Path(uri.path))

                image_size = os.path.getsize(self.source_dir / uri.path)

                image["bufferView"] = len(self.description["bufferViews"])
                self.description["bufferViews"].append(
                    {
                        "buffer": 0,
                        "byteLength": image_size,
                        "byteOffset": self.bin_length,
                    }
                )
                del image["uri"]

                self.bin_length += image_size

    def write(self, file: BinaryIO) -> None:
        """Write a GLB file."""
        description_json = json.dumps(self.description, separators=(",", ":")).encode(
            "utf-8"
        )

        # A GLB file has a JSON chunk and a BIN chunk, each of which need to be aligned to a 4-byte boundary
        aligned_json_length = align_to_4(len(description_json))
        json_padding = b" " * (aligned_json_length - len(description_json))

        aligned_bin_length = align_to_4(self.bin_length)
        bin_padding = b"\x00" * (aligned_bin_length - self.bin_length)

        # write header
        # magic number 0x46546C67
        file.write(b"glTF")
        # version 2
        file.write((2).to_bytes(4, byteorder="little"))
        # length (12 byte header + 8 byte chunk header + padded JSON + 8 byte chunk header + padded BIN)
        length = 12 + 8 + aligned_json_length + 8 + aligned_bin_length
        file.write(length.to_bytes(4, byteorder="little"))

        # JSON chunk
        # chunkLength
        file.write(aligned_json_length.to_bytes(4, byteorder="little"))
        # chunkType 0x4E4F534A
        file.write(b"JSON")
        # chunkData
        file.write(description_json)
        file.write(json_padding)

        # BIN chunk
        # chunkLength
        file.write(aligned_bin_length.to_bytes(4, byteorder="little"))
        # chunkType 0x004E4942
        file.write(b"BIN\x00")
        # chunkData
        for source_file_name in self.source_files:
            with open(self.source_dir / source_file_name, "rb") as source_file:
                while True:
                    read = source_file.read(4096)
                    if len(read) == 0:
                        break
                    file.write(read)
        file.write(bin_padding)


def pack(gltf: GLTFDescription, source_dir: Path, dest: Path) -> None:
    glb = GLB(gltf, source_dir)
    glb.pack()
    with open(dest, "wb") as f:
        glb.write(f)


def main():
    logging.basicConfig(
        format="%(asctime)s %(module)s: %(message)s", level=logging.INFO
    )
    parser = argparse.ArgumentParser(
        description="Takes a GLTF file and packs it and all external files into a binary GLB file",
    )
    parser.add_argument("input", help="The name of the GLTF file to pack")
    parser.add_argument("output", help="The output filename to be saved")
    args = parser.parse_args()

    with open(args.input, "r") as f:
        gltf = json.load(f)
        input_dir = Path(args.input).parent.resolve()
        pack(gltf, input_dir, Path(args.output))
    pass


if __name__ == "__main__":
    sys.exit(main())
