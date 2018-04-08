About Mimesis
-------------

Mimesis is a C++ library for RFC2822 message parsing and creation. It was born
out of a frustration with existing email libraries for C++. The goals of
Mimesis are:

- Equally good at parsing and building RFC2822 messages.
- Easy API without unnecessary abstraction layers.
- Make good use of C++11 features.

In particular, applications that parse/build emails want to treat them just
like a user would treat emails in a mail user agent (MUA). Users don't see
emails as a tree of MIME structures; instead they see them as a collection of
headers, bodies and attachments. Users also don't see details such as MIME
types and content encodings, MUAs handle those details automatically.
