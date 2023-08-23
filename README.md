# SMTA
Streaming Media Transcoding Application

The purpose of the Streaming Media Transcoding Application (SMTA) is to demonstrate video transcoding capabilities of Intel® HD Graphics enabled processors, particularly in terms of stream density (number of streams per “system”). The SMTA takes an input AV stream from a Real Time Streaming Protocol (RTSP) source, splits out the video, decodes it, optionally applies some processing to the video frames (scaling, de-interlacing).  Finally, it re-encodes the result to the specified encoding, multiplexes it with audio (if configured), and then transmits it to the designated host and port(s) using Real-time Transport Protocol (RTP).
