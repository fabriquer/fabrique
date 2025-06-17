use ariadne::{Color, Label, Report, ReportKind, Source};
use chumsky::Parser as _;
use clap::Parser as ClapParser;
use std::path::PathBuf;

mod parser;

#[derive(Debug, ClapParser)]
struct CliArgs {
    /// Fabrique source file or directory containing fabfile
    filename: PathBuf,

    /// Output a representation of the AST after parsing
    #[arg(long)]
    dump_ast: bool,
}

fn main() {
    let cli_args = CliArgs::parse();
    let filename = cli_args.filename.display().to_string();

    match std::fs::read_to_string(&filename) {
        Ok(src) => {
            if let Some(ast) = parse(&filename, &src) {
                if cli_args.dump_ast {
                    println!("{:#?}", ast);
                }
            }
        }
        Err(e) => {
            println!("Error: {}", e);
        }
    }
}

fn parse<'src>(filename: &'src str, src: &'src str) -> Option<parser::expr::Expr<'src>> {
    let parse_result = parser::expr::expression().parse(&src);

    for err in parse_result.errors() {
        let range = err.span().into_range();
        let reason = err.reason();

        let summary = if let Some(found) = reason.found() {
            format!("Unexpected '{}'", found)
        } else {
            reason.to_string()
        };

        let mut report = Report::build(ReportKind::Error, (filename, range.clone()))
            .with_config(ariadne::Config::new().with_index_type(ariadne::IndexType::Byte))
            .with_message(summary)
            .with_label(
                Label::new((filename, range.clone()))
                    .with_message(err.reason().to_string())
                    .with_color(Color::Red),
            );

        for ctx in err.contexts() {
            report = report.with_label(
                Label::new((filename, range.clone()))
                    .with_message(format!("{:#?}", ctx)),
            );
        }

        report
            .finish()
            .eprint((filename, Source::from(&src)))
            .unwrap();
    }

    parse_result.output().cloned()
}
