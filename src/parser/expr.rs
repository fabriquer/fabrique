use chumsky::prelude::*;

#[derive(Clone, Debug, PartialEq)]
pub enum Expr<'src> {
    Bool(bool),
    Int(i64),

    Name(&'src str),
    Ref(&'src str),

    Neg(Box<Expr<'src>>),
    Add(Box<Expr<'src>>, Box<Expr<'src>>),
    Sub(Box<Expr<'src>>, Box<Expr<'src>>),
    Mul(Box<Expr<'src>>, Box<Expr<'src>>),
    Div(Box<Expr<'src>>, Box<Expr<'src>>),

    Call {
        target: Box<Expr<'src>>,
        arguments: Vec<Expr<'src>>,
    },

    Fn {
        params: Vec<Expr<'src>>,
        body: Box<Expr<'src>>,
    },
}

pub fn expression<'src>()
-> impl Parser<'src, &'src str, Expr<'src>, chumsky::extra::Err<Rich<'src, char>>> {
    let identifier = text::unicode::ident().padded();

    recursive(|expr| {
        let boolean = choice((
            just("true").to(Expr::Bool(true)),
            just("false").to(Expr::Bool(false)),
        ))
        .padded();

        let integer = text::int(10)
            .map(|s: &str| Expr::Int(s.parse().unwrap()))
            .padded();

        let name_ref = identifier.map(Expr::Ref);

        let call = expr
            .clone()
            .then(
                expr.clone()
                    .separated_by(just(','))
                    .allow_trailing()
                    .collect::<Vec<_>>()
                    .delimited_by(just('('), just(')')),
            )
            .map(|(target, args)| Expr::Call {
                target: Box::new(target),
                arguments: args,
            });

        let atom = choice((
            boolean,
            integer,
            name_ref,
            expr.delimited_by(just('('), just(')')),
            call,
        ));

        let op = |c| just(c).padded();

        let unary = op('-')
            .repeated()
            .foldr(atom, |_op, rhs| Expr::Neg(Box::new(rhs)));

        let product = unary.clone().foldl(
            choice((
                op('*').to(Expr::Mul as fn(_, _) -> _),
                op('/').to(Expr::Div as fn(_, _) -> _),
            ))
            .then(unary)
            .repeated(),
            |lhs, (op, rhs)| op(Box::new(lhs), Box::new(rhs)),
        );

        let sum = product.clone().foldl(
            choice((
                op('+').to(Expr::Add as fn(_, _) -> _),
                op('-').to(Expr::Sub as fn(_, _) -> _),
            ))
            .then(product)
            .repeated(),
            |lhs, (op, rhs)| op(Box::new(lhs), Box::new(rhs)),
        );

        sum
    })
}

#[cfg(test)]
mod test {
    use super::*;

    fn boxed<'src>(val: Expr<'src>) -> Box<Expr<'src>> {
        Box::new(val)
    }

    fn check_expr(src: &str, expected: Expr) {
        assert_eq!(expression().parse(src).into_result(), Ok(expected));
    }

    #[test]
    fn test_integer_literal() {
        check_expr("1", Expr::Int(1));
        check_expr(" 2   ", Expr::Int(2));
        check_expr(" 34 ", Expr::Int(34));
    }

    #[test]
    fn test_negation() {
        check_expr("-1", Expr::Neg(boxed(Expr::Int(1))));
        check_expr(" --2 ", Expr::Neg(boxed(Expr::Neg(boxed(Expr::Int(2))))));
    }

    #[test]
    fn test_addition() {
        check_expr("1+2", Expr::Add(boxed(Expr::Int(1)), boxed(Expr::Int(2))));
        check_expr(
            " 3 + 4 ",
            Expr::Add(boxed(Expr::Int(3)), boxed(Expr::Int(4))),
        );
        check_expr(
            " -5 + -6 ",
            Expr::Add(
                boxed(Expr::Neg(boxed(Expr::Int(5)))),
                boxed(Expr::Neg(boxed(Expr::Int(6)))),
            ),
        );
    }

    #[test]
    fn test_subtraction() {
        check_expr("1-2", Expr::Sub(boxed(Expr::Int(1)), boxed(Expr::Int(2))));
        check_expr(
            " 3 - 4 ",
            Expr::Sub(boxed(Expr::Int(3)), boxed(Expr::Int(4))),
        );
        check_expr(
            " -5 - -6 ",
            Expr::Sub(
                boxed(Expr::Neg(boxed(Expr::Int(5)))),
                boxed(Expr::Neg(boxed(Expr::Int(6)))),
            ),
        );
    }

    #[test]
    fn test_multiplication() {
        check_expr("1*2", Expr::Mul(boxed(Expr::Int(1)), boxed(Expr::Int(2))));
        check_expr(
            " 3 * 4 ",
            Expr::Mul(boxed(Expr::Int(3)), boxed(Expr::Int(4))),
        );
        check_expr(
            " -5 * -6 ",
            Expr::Mul(
                boxed(Expr::Neg(boxed(Expr::Int(5)))),
                boxed(Expr::Neg(boxed(Expr::Int(6)))),
            ),
        );
    }

    #[test]
    fn test_division() {
        check_expr("1/2", Expr::Div(boxed(Expr::Int(1)), boxed(Expr::Int(2))));
        check_expr(
            " 3 / 4 ",
            Expr::Div(boxed(Expr::Int(3)), boxed(Expr::Int(4))),
        );
        check_expr(
            " -5 / -6 ",
            Expr::Div(
                boxed(Expr::Neg(boxed(Expr::Int(5)))),
                boxed(Expr::Neg(boxed(Expr::Int(6)))),
            ),
        );
    }

    #[test]
    fn test_sum_of_product() {
        check_expr(
            "1*2+3*4",
            Expr::Add(
                boxed(Expr::Mul(boxed(Expr::Int(1)), boxed(Expr::Int(2)))),
                boxed(Expr::Mul(boxed(Expr::Int(3)), boxed(Expr::Int(4)))),
            ),
        );

        check_expr(
            " 3 * 4 + 5 * 42 ",
            Expr::Add(
                boxed(Expr::Mul(boxed(Expr::Int(3)), boxed(Expr::Int(4)))),
                boxed(Expr::Mul(boxed(Expr::Int(5)), boxed(Expr::Int(42)))),
            ),
        );

        check_expr(
            " 10 + 20 * 30 + 40 * 50 + 60 ",
            Expr::Add(
                boxed(Expr::Add(
                    boxed(Expr::Add(
                        boxed(Expr::Int(10)),
                        boxed(Expr::Mul(boxed(Expr::Int(20)), boxed(Expr::Int(30)))),
                    )),
                    boxed(Expr::Mul(boxed(Expr::Int(40)), boxed(Expr::Int(50)))),
                )),
                boxed(Expr::Int(60)),
            ),
        );
    }

    #[test]
    fn test_parentheses() {
        check_expr(
            "(1 + 2) * (3 + 4)",
            Expr::Mul(
                boxed(Expr::Add(boxed(Expr::Int(1)), boxed(Expr::Int(2)))),
                boxed(Expr::Add(boxed(Expr::Int(3)), boxed(Expr::Int(4)))),
            ),
        );
    }

    #[test]
    fn test_name_ref() {
        check_expr("foo", Expr::Ref("foo".into()));
        check_expr(
            " supercalifragilisticexpialidocious ",
            Expr::Ref("supercalifragilisticexpialidocious".into()),
        );
    }
}
