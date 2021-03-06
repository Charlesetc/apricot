/* silver token */

open Silver_utils;

type token =
  | Identifier of string
  | String_literal of string
  | Dot_literal of string
  | Left_curly
  | Right_curly
  | Left_round
  | Right_round
  | Left_angle
  | Right_angle
  | Dot
  | Newline
  | Colon
  | Comma
  | Space
  | Unit;

let string_of_token tokens =>
  switch tokens {
  | Left_curly => "left curly"
  | Right_curly => "right curly"
  | Left_round => "left round"
  | Right_round => "right round"
  | Left_angle => "left angle"
  | Right_angle => "right angle"
  | Dot => "dot"
  | Space => "space"
  | Colon => "colon"
  | Comma => "comma"
  | Identifier s => ":" ^ s
  | Dot_literal s => "." ^ s
  | String_literal s => "\"" ^ s ^ "\""
  | Newline => "newline"
  | Unit => "()"
  | _ => "<unknown token!>"
  };

let is_split_char c => {
  let split_chars = ":;\"\n'{}()<>., ";
  switch (String.index split_chars c) {
  | exception Not_found => false
  | _ => true
  }
};

let next_character_generator position stream () => {
  let char = Stream.next stream;
  if (char == '\n') {
    position := {line: (!position).line + 1, column: 0}
  } else {
    position := {line: (!position).line, column: (!position).column + 1}
  };
  ()
};

let token stream => {
  let position = ref {line: 0, column: (-1)};
  let advance_character = next_character_generator position stream;
  let error_with_position desc => raise (Silver_utils.Silver_error desc !position);
  let rec read_string quote => {
    let starting_position = !position;
    switch (Stream.peek stream) {
    | Some c when c == quote => ""
    | Some c =>
      advance_character ();
      switch (Silver_utils.string_of_char c ^ read_string quote) {
      | s => s
      /* This catches exceptions and rethrows them with the proper position */
      | exception Silver_utils.Silver_error desc pos =>
        raise (Silver_utils.Silver_error desc starting_position)
      }
    | None => error_with_position "hit end of file when parsing string"
    }
  };
  let rec read_identifier () =>
    switch (Stream.peek stream) {
    | None => ""
    | Some c when is_split_char c => ""
    | Some c =>
      advance_character ();
      Silver_utils.string_of_char c ^ read_identifier ()
    };
  let gen_token tok =>
    Some {
      advance_character ();
      tok
    };
  let stream_fn _ => {
    let character =
      switch (Stream.peek stream) {
      | Some '}' => gen_token Right_curly
      | Some '{' => gen_token Left_curly
      | Some ':' => gen_token Colon
      | Some ',' => gen_token Comma
      | Some ')' => gen_token Right_round
      | Some '(' => gen_token Left_round
      | Some '>' => gen_token Right_angle
      | Some '<' => gen_token Left_angle
      | Some '\n' => gen_token Newline
      | Some ';' => gen_token Newline
      | Some ' ' => gen_token Space
      | Some '.' =>
        advance_character ();
        Some (
          switch (Stream.peek stream) {
          | None => Dot
          | Some c when is_split_char c => Dot
          | Some c => Dot_literal (read_identifier ())
          }
        )
      | Some '"' =>
        advance_character ();
        let char = String_literal (read_string '"');
        advance_character ();
        Some char
      | Some '\'' =>
        advance_character ();
        let char = String_literal (read_string '\'');
        advance_character ();
        Some char
      | Some _ => Some (Identifier (read_identifier ()))
      | None => None
      };
    switch character {
    | Some c => Some (c, !position)
    | None => None
    }
  };
  Stream.from stream_fn
};
