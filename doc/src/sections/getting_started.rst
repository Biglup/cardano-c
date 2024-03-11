Getting Started
===============

Welcome to the Cardano C Library documentation. This section will guide you through the steps necessary to start using the Cardano C Library in your projects.

Building and Installing from Source
-----------------------------------

**Prerequisites**:
 - C99 compiler
 - CMake 2.8 or newer
 - CMake compatible build system (make, Apple Xcode, MinGW, ...)

Configuration Options:

You can configure the build with several options using CMake. The most notable options for the Cardano C Library include enabling shared libraries, adjusting the installation prefix, and enabling unit tests.

**Configuration options**

A handful of configuration flags can be passed to `cmake`. The following table lists libcardano-c compile-time directives and several important generic flags.

========================  =======================================================   ======================  =====================================================================================================================
Option                    Meaning                                                   Default                 Possible values
------------------------  -------------------------------------------------------   ----------------------  ---------------------------------------------------------------------------------------------------------------------
``CMAKE_C_COMPILER``      C compiler to use                                         ``cc``                   ``gcc``, ``clang``, ``clang-3.5``, ...
``CMAKE_INSTALL_PREFIX``  Installation prefix                                       System-dependent         ``/usr/local/lib``, ...
``DOXYGEN_ENABLED``       Generate this documentation                               ``OFF``                  ``ON``, ``OFF``
``TESTING_ENABLED``       Build unit tests                                          ``OFF``                  ``ON``, ``OFF``
========================  =======================================================   ======================  =====================================================================================================================

The following configuration options will also be defined as macros [#]_ in ``config.h`` and can therefore be used in code:

==========================   =======================================================   ======================  =====================================================================================================================
Option                       Meaning                                                   Default                 Possible values
--------------------------   -------------------------------------------------------   ----------------------  ---------------------------------------------------------------------------------------------------------------------
``COLLECTION_GROW_FACTOR``   Factor for buffer growth & shrinking                      ``1.5``                 Decimals > 1
==========================   =======================================================   ======================  =====================================================================================================================

.. [#] ``ON`` & ``OFF`` will be translated to ``1`` and ``0`` using `cmakedefine <https://cmake.org/cmake/help/v3.2/command/configure_file.html?highlight=cmakedefine>`_.

If you want to pass other custom configuration options, please refer to `<http://www.cmake.org/Wiki/CMake_Useful_Variables>`_.

**Building using Make**:

To build the Cardano C Library as a static library, use the following commands:

.. code-block:: bash

  cd $(mktemp -d)
  cmake -DCMAKE_BUILD_TYPE=Release /path/to/cardano-c
  make

**Note**: Replace `/path/to/cardano-c` with the actual path to the Cardano C Library source directory.

To install locally:

.. code-block:: bash

  make install

Root permissions are required on most systems when using the default installation prefix.

**Portability**

libcardano-c is highly portable and works on both little- and big-endian systems regardless of the operating system. After building
on an exotic platform, you might wish to verify the result by running the unit tests.

Linking with libcardano-c
-------------------------

If you include and linker paths include the directories to which libcardano-c has been installed, compiling programs that uses libcardano-c requires
no extra considerations.

You can verify that everything has been set up properly by creating a file with the following contents

.. code-block:: c

    #include <cardano/cardano.h>
    #include <stdio.h>

    int main(int argc, char * argv[])
    {
        printf("Hello from libcardano-c %s\n", LIB_CARDANO_C_VERSION);
    }


and compiling it

.. code-block:: bash

    cc hello_cardano.c -lcardano-c -o hello_cardano


libcardano-c also comes with `pkg-config <https://wiki.freedesktop.org/www/Software/pkg-config/>`_ support. If you install libcardano-c with a custom prefix, you can use pkg-config to resolve the headers and objects:

.. code-block:: bash

    cc $(pkg-config --cflags libcardano-c) hello_cardano.c $(pkg-config --libs libcardano-c) -o hello_cardano
