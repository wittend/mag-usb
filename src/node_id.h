#ifndef NODE_ID_H
#define NODE_ID_H

/* Longest node_id accepted from config.toml or -N. */
#define NODE_ID_MAX_LEN 65

/* Returns 1 if s is a valid node_id, 0 otherwise.
 * A valid node_id is 1 to NODE_ID_MAX_LEN printable ASCII characters:
 * letters, digits and punctuation.  Spaces are not allowed, and neither are
 * double quote and backslash, which would need escaping in the JSON config
 * message. */
int node_id_is_valid(const char *s);

#endif // NODE_ID_H
