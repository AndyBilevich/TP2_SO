docker run -v ${PWD}:/root --rm --name docker_so --security-opt seccomp:unconfined -ti agodio/itba-so:1.0 /root/.dckr_make $args