(* Command line handling for OCaml tools in libguestfs.
 * Copyright (C) 2016 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *)

type spec =
  | Unit of (unit -> unit)
    (* Simple option with no argument; call the function. *)
  | Set of bool ref
    (* Simple option with no argument; set the reference to true. *)
  | Clear of bool ref
    (* Simple option with no argument; set the reference to false. *)
  | String of string * (string -> unit)
    (* Option requiring an argument; the first element in the tuple
       is the documentation string of the argument, and the second
       is the function to call. *)
  | Set_string of string * string ref
    (* Option requiring an argument; the first element in the tuple
       is the documentation string of the argument, and the second
       is the reference to be set. *)
  | Int of string * (int -> unit)
    (* Option requiring an integer value as argument; the first
       element in the tuple is the documentation string of the
       argument, and the second is the function to call. *)
  | Set_int of string * int ref
    (* Option requiring an integer value as argument; the first
       element in the tuple is the documentation string of the
       argument, and the second is the reference to be set. *)

type keys = string list
type doc = string
type usage_msg = string
type anon_fun = (string -> unit)

type speclist = (keys * spec * doc) list

val compare_command_line_args : string -> string -> int
(** Compare command line arguments for equality, ignoring any leading [-]s. *)

type t
(** The abstract data type. *)

val create : speclist -> ?anon_fun:anon_fun -> usage_msg -> t
(** [Getopt.create speclist ?anon_fun usage_msg] creates a new parses
    for command line arguments.

    [speclist] is a list of triples [(keys, spec, doc)]: [keys] is a
    list of options, [spec] is the associated action, and [doc] is
    the help text.

    [anon_fun] is an optional function to handle non-option arguments;
    not specifying one means that only options are allowed, and
    non-options will cause an error.

    [usage_msg] is the string which is printed before the list of
    options as help text.
*)

val parse_argv : t -> string array -> unit
(** [Getopt.parse handle args] parses the specified arguments.

    [handle] is the [Getopt.t] type with the configuration of the
    command line arguments.

    [args] is the array with command line arguments, with the first
    element representing the application name/path.

    In case of errors, like non-integer value for [Int] or [Set_int],
    an error message is printed, together with a pointer to use
    [--help], and then the program exists with a non-zero exit
    value. *)

val parse : t -> unit
(** Call {!Getopt.parse_argv} on [Sys.argv]. *)
