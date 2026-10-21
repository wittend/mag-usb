#ifndef NODE_ID_H
#define NODE_ID_H

/* Longest node_id accepted from config.toml or -N. */
#define NODE_ID_MAX_LEN 65

/* Longest topic prefix accepted from config.toml. */
#define MQTT_TOPIC_PREFIX_MAX_LEN 128

/* Returns 1 if s is a valid node_id, 0 otherwise.
 * A valid node_id is 1 to NODE_ID_MAX_LEN printable ASCII characters:
 * letters, digits and punctuation.  Spaces are not allowed, and neither are
 * double quote and backslash, which would need escaping in the JSON config
 * message.  Slash, plus and hash are refused too, because the node_id becomes
 * a topic level. */
int node_id_is_valid(const char *s);

/* Returns 1 if s can be the base of the station topics, for example
 * "hamsci/mag".  Levels are separated by single slashes, with no leading or
 * trailing slash, no wildcards and no spaces. */
int mqtt_topic_prefix_is_valid(const char *s);

#endif // NODE_ID_H
