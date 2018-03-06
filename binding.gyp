{
  "targets": [
    {
      "target_name": "clockstamp",
      "cflags": [
        "-fexceptions",
        "-std=c++11",
        "-pthread"
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
