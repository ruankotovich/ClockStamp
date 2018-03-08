{
  "targets": [
    {
      "target_name": "clockstamp",
      "cflags": [
        "-fexceptions",
        "-std=c++11",
        "-pthread",
	"-DDEBUG"
      ],
      "cflags_cc!": [
        "-fno-exceptions"
      ],
      "sources": [
        "./clockstamp.cpp",
      ]
    }
  ]
}
