# Copyright (c) 2023-2026 Roland Metivier <metivier.roland@chlorophyt.us>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

import hashlib
import logging
from typing import Callable

import pytest
from pytest_embedded_idf.dut import IdfDut
from pytest_embedded_qemu.app import QemuApp
from pytest_embedded_qemu.dut import QemuDut


@pytest.mark.supported_targets
@pytest.mark.generic
def test_hello_world(
    dut: IdfDut, log_minimum_free_heap_size: Callable[..., None]
) -> None:
    dut.expect('Hello world!')
    log_minimum_free_heap_size()


def verify_elf_sha256_embedding(app: QemuApp, sha256_reported: str) -> None:
    sha256 = hashlib.sha256()
    with open(app.elf_file, 'rb') as f:
        sha256.update(f.read())
    sha256_expected = sha256.hexdigest()

    logging.info(f'ELF file SHA256: {sha256_expected}')
    logging.info(f'ELF file SHA256 (reported by the app): {sha256_reported}')

    # the app reports only the first several hex characters of the SHA256, check that they match
    if not sha256_expected.startswith(sha256_reported):
        raise ValueError('ELF file SHA256 mismatch')


@pytest.mark.esp32  # we only support qemu on esp32 for now
@pytest.mark.host_test
@pytest.mark.qemu
def test_hello_world_host(app: QemuApp, dut: QemuDut) -> None:
    sha256_reported = (
        dut.expect(r'ELF file SHA256:\s+([a-f0-9]+)').group(1).decode('utf-8')
    )
    verify_elf_sha256_embedding(app, sha256_reported)

    dut.expect('Hello world!')
