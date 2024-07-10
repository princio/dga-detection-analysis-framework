import {
  Column,
  Entity,
  Index,
  OneToMany,
  PrimaryGeneratedColumn,
} from "typeorm";
import { DnNn } from "./DnNn";

@Index("nns_pkey", ["id"], { unique: true })
@Index("nns_name_key", ["name"], { unique: true })
@Entity("nn", { schema: "public" })
export class Nn {
  @PrimaryGeneratedColumn({ type: "integer", name: "id" })
  id: number;

  @Column("text", { name: "name", unique: true })
  name: string;

  @Column("integer", { name: "folds", nullable: true })
  folds: number | null;

  @Column("integer", { name: "epochs", nullable: true })
  epochs: number | null;

  @Column("integer", { name: "fold", nullable: true })
  fold: number | null;

  @Column("integer", { name: "epoch", nullable: true })
  epoch: number | null;

  @Column("integer", { name: "max_lenght_input" })
  maxLenghtInput: number;

  @Column("real", { name: "units", precision: 24 })
  units: number;

  @Column("text", { name: "directory" })
  directory: string;

  @Column("enum", {
    name: "extractor",
    nullable: true,
    enum: [
      "domain",
      "any",
      "nosfx",
      "domain_sfx",
      "icann",
      "private",
      "none",
      "tld",
    ],
  })
  extractor:
    | "domain"
    | "any"
    | "nosfx"
    | "domain_sfx"
    | "icann"
    | "private"
    | "none"
    | "tld"
    | null;

  @OneToMany(() => DnNn, (dnNn) => dnNn.nn)
  dnNns: DnNn[];
}
