# Too many configs...

JS projects have to many configs. It's hard to manage. This project fixes that.

## How?

1. create `config.js` that contains configuration for a lot of files
2. the cli symlinks files like `.eslintrc` to `/dev/jscfgi0`. this is a char device created by a custom kernel module
3. any tool on the system that will try to read `.eslintrc` (and friends) will be intercepted by the kernel module
4. the kernel module will talk to a daemon that keeps track of the state of your projects and knows about your `config.js` and projects nesting
5. the daemon responds with the text that should be contained in the `.eslintrc`
