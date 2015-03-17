Fabrique
=========

[![Build Status](http://jack.jonandchrissy.ca/jenkins/buildStatus/icon?job=Fabrique)](https://jack.jonandchrissy.ca/jenkins/job/Fabrique)

Fabrique is a build language for constructing complex systems.
It is functional, statically typed, has very few dependencies and is
designed for either command-line use or IDE integration.

Fabrique can generate a few kinds of output:
 * [Ninja](http://martine.github.io/ninja)
 * [POSIX Make](http://pubs.opengroup.org/onlinepubs/009695399/utilities/make.html) (no BSD or GNU extensions)
 * [GraphViz](http://www.graphviz.org/)
 * [POSIX (Bourne) shell](http://pubs.opengroup.org/onlinepubs/009695399/utilities/xcu_chap02.html) ([TODO](https://github.com/fabriquer/fabrique/issues/1))

The last of these will be especially helpful for bootstrapping.
