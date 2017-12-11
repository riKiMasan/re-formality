module SignupForm = {
  type field =
    | Email
    | Password
    | PasswordConfirmation;
  type state = {
    email: string,
    password: string,
    passwordConfirmation: string
  };
  let get = (field, state) =>
    switch field {
    | Email => state.email
    | Password => state.password
    | PasswordConfirmation => state.passwordConfirmation
    };
  let update = ((field, value), state) =>
    switch (field, value) {
    | (Email, value) => {...state, email: value}
    | (Password, value) => {...state, password: value}
    | (PasswordConfirmation, value) => {...state, passwordConfirmation: value}
    };
  let strategy = Formality.Strategy.OnFirstSuccessOrFirstBlur;
  let asyncStrategy = Some(Formality.AsyncStrategy.OnChange);
  module Validators =
    Formality.MakeValidators(
      {
        type t = field;
      }
    );
  type validators = Validators.t(Formality.validator(field, state));
  let validators =
    Formality.(
      Validators.empty
      |> Validators.add(
           Email,
           {
             strategy: None, /* None means global will be used */
             dependents: None,
             validate: (value', state) => {
               let emailRegex = [%bs.re {|/.*@.*\..+/|}];
               let value = value' |> Js.Option.getWithDefault(state.email);
               switch value {
               | "" =>
                 ValidityBag({
                   valid: false,
                   tag: None,
                   message: Some("Email is required")
                 })
               | _ when ! (emailRegex |> Js.Re.test(value)) =>
                 ValidityBag({
                   valid: false,
                   tag: None,
                   message: Some("Email is invalid")
                 })
               | _ =>
                 ValidityBag({valid: true, tag: None, message: Some("Nice!")})
               };
             },
             validateAsync:
               Some(
                 value =>
                   Js.Promise.(
                     value
                     |> Api.validateEmail
                     |> then_(valid =>
                          valid ?
                            resolve(
                              ValidityBag({
                                valid: true,
                                tag: None,
                                message: Some("Nice!")
                              })
                            ) :
                            resolve(
                              ValidityBag({
                                valid: false,
                                tag: None,
                                message: Some("Email is already taken")
                              })
                            )
                        )
                   )
               )
           }
         )
      |> Validators.add(
           Password,
           {
             strategy: None, /* None means global will be used */
             dependents: Some([PasswordConfirmation]),
             validate: (value', state) => {
               let minLength = 4;
               let strongLength = 6;
               let value = value' |> Js.Option.getWithDefault(state.password);
               switch value {
               | "" =>
                 ValidityBag({
                   valid: false,
                   tag: None,
                   message: Some("Password is required")
                 })
               | _ when String.length(value) < minLength =>
                 ValidityBag({
                   valid: false,
                   tag: None,
                   message: Some({j| $(minLength)+ characters, please|j})
                 })
               | _ when String.length(value) < strongLength =>
                 ValidityBag({
                   valid: true,
                   tag: Some("weak"),
                   message: Some("Can be stronger... (still valid tho)")
                 })
               | _ =>
                 ValidityBag({valid: true, tag: None, message: Some("Nice!")})
               };
             },
             validateAsync: None
           }
         )
      |> Validators.add(
           PasswordConfirmation,
           {
             strategy: None,
             dependents: None,
             validate: (value', state) => {
               let value =
                 value' |> Js.Option.getWithDefault(state.passwordConfirmation);
               switch value {
               | "" =>
                 ValidityBag({
                   valid: false,
                   tag: None,
                   message: Some("Confirmation is required")
                 })
               | _ when value !== state.password =>
                 ValidityBag({
                   valid: false,
                   tag: None,
                   message: Some("Password doesn't match")
                 })
               | _ =>
                 ValidityBag({valid: true, tag: None, message: Some("Match!")})
               };
             },
             validateAsync: None
           }
         )
    );
  exception InvalidResult(field);
};

module Container = Formality.Make(SignupForm);

let component = ReasonReact.statelessComponent("SignupForm");

let make = (_) => {
  ...component,
  render: (_) =>
    <Container
      initialState={email: "", password: "", passwordConfirmation: ""}
      onSubmit=(
        (~notifyOnSuccess, ~notifyOnFailure, state) => {
          Js.log("Called with:");
          Js.log(state);
          Js.log("If api returned error this callback should be called:");
          Js.log(notifyOnFailure);
          let _ = Js.Global.setTimeout(notifyOnSuccess, 500);
          ();
        }
      )>
      ...(
           ({state, results, update, blur, validating, submitting, submit}) =>
             <form className="form" onSubmit=submit>
               <div className="form-messages-area form-messages-area-lg" />
               <div className="form-content">
                 <h2 className="push-lg">
                   ("Signup" |> ReasonReact.stringToElement)
                 </h2>
                 <div className="form-row">
                   <label htmlFor="signup--email" className="label-lg">
                     ("Email" |> ReasonReact.stringToElement)
                   </label>
                   <input
                     id="signup--email"
                     value=state.email
                     disabled=(submitting |> Js.Boolean.to_js_boolean)
                     onChange=(update(SignupForm.Email))
                     onBlur=(blur(SignupForm.Email))
                   />
                   (
                     switch (
                       SignupForm.Email |> results,
                       SignupForm.Email |> validating
                     ) {
                     | (_, true) =>
                       <div className="form-message">
                         ("Checking..." |> ReasonReact.stringToElement)
                       </div>
                     | (Some(result), false) =>
                       switch result {
                       | Formality.ValidityBag(validity) =>
                         <div
                           className=(
                             Cn.make([
                               "form-message",
                               validity.valid ? "success" : "failure"
                             ])
                           )>
                           (
                             validity.message
                             |> Js.Option.getExn
                             |> ReasonReact.stringToElement
                           )
                         </div>
                       | _ => raise(SignupForm.InvalidResult(SignupForm.Email))
                       }
                     | (None, false) => ReasonReact.nullElement
                     }
                   )
                 </div>
                 <div className="form-row">
                   <label htmlFor="signup--password" className="label-lg">
                     ("Password" |> ReasonReact.stringToElement)
                   </label>
                   <input
                     id="signup--password"
                     value=state.password
                     disabled=(submitting |> Js.Boolean.to_js_boolean)
                     onChange=(update(SignupForm.Password))
                     onBlur=(blur(SignupForm.Password))
                   />
                   (
                     switch (SignupForm.Password |> results) {
                     | Some(result) =>
                       switch result {
                       | Formality.ValidityBag(validity) =>
                         <div
                           className=(
                             Cn.make([
                               "form-message",
                               validity.valid ?
                                 switch validity.tag {
                                 | Some(tag) => tag
                                 | None => "success"
                                 } :
                                 "failure"
                             ])
                           )>
                           (
                             validity.message
                             |> Js.Option.getExn
                             |> ReasonReact.stringToElement
                           )
                         </div>
                       | _ =>
                         raise(SignupForm.InvalidResult(SignupForm.Password))
                       }
                     | None => ReasonReact.nullElement
                     }
                   )
                 </div>
                 <div className="form-row">
                   <label
                     htmlFor="signup--passwordConfirmation"
                     className="label-lg">
                     ("Confirmation" |> ReasonReact.stringToElement)
                   </label>
                   <input
                     id="signup--passwordConfirmation"
                     value=state.passwordConfirmation
                     disabled=(submitting |> Js.Boolean.to_js_boolean)
                     onChange=(update(SignupForm.PasswordConfirmation))
                     onBlur=(blur(SignupForm.PasswordConfirmation))
                   />
                   (
                     switch (SignupForm.PasswordConfirmation |> results) {
                     | Some(result) =>
                       switch result {
                       | Formality.ValidityBag(validity) =>
                         <div
                           className=(
                             Cn.make([
                               "form-message",
                               validity.valid ? "success" : "failure"
                             ])
                           )>
                           (
                             validity.message
                             |> Js.Option.getExn
                             |> ReasonReact.stringToElement
                           )
                         </div>
                       | _ =>
                         raise(
                           SignupForm.InvalidResult(
                             SignupForm.PasswordConfirmation
                           )
                         )
                       }
                     | None => ReasonReact.nullElement
                     }
                   )
                 </div>
                 <div className="form-row">
                   <button
                     className="push-lg"
                     disabled=(submitting |> Js.Boolean.to_js_boolean)>
                     (
                       (submitting ? "Submitting..." : "Submit")
                       |> ReasonReact.stringToElement
                     )
                   </button>
                 </div>
               </div>
             </form>
         )
    </Container>
};
