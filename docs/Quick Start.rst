Quick Start
===========

Introduction
------------

baregear is a programming language compiler created by First Person
on 10 July 2026. It was originally named 'mpdc' (Machine Program
Definition Compiler), then 'pdc' (Program Defined Compiler), before
finally becoming 'baregear' to avoid confusion with existing software.

baregear is designed to produce high-performance, stable executables
from alpha-stage programs, focusing on eliminating unexpected crashes
and providing clear feedback instead of failing silently. Its key
features include AI-powered bug fixing, built-in feature enabling and
disabling, and dynamic typing with optional static types.

Above all, baregear is made to be easy to read and write, keeping
syntax errors to a minimum so you can spend more time on your program
and less on fighting the compiler.

Writing Simple 'Hello World' Program
------------------------------------

.. code-block:: baregear

    print "Hello World!"

- Replace 'Hello World' With text if you want And Save The File On Disk.

.. code-block:: bash

    $ baregear helloWorld.br -o helloWorld
    $ ./helloWorld

- Replace helloWorld.br with actual file name.

prints Hello World Or Any Text:

.. code-block:: bash

    Hello World!
