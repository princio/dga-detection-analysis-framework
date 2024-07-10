import {
  Column,
  Entity,
  Index,
  JoinColumn,
  ManyToOne,
  PrimaryGeneratedColumn,
} from "typeorm";
import { Dn } from "./Dn";
import { Nn } from "./Nn";

@Index("dn_nn__dn_id_nn_id___unique", ["dnId", "nnId"], { unique: true })
@Index("dn_nn_dn_id_index", ["dnId"], {})
@Index("fki_dn_nn___dn_id", ["dnId"], {})
@Index("dn_nn_pkey", ["id"], { unique: true })
@Entity("dn_nn", { schema: "public" })
export class DnNn {
  @PrimaryGeneratedColumn({ type: "integer", name: "id" })
  id: number;

  @Column("bigint", { name: "dn_id", unique: true })
  dnId: string;

  @Column("bigint", { name: "nn_id", unique: true })
  nnId: string;

  @Column("double precision", { name: "value", nullable: true, precision: 53 })
  value: number | null;

  @Column("double precision", { name: "logit", nullable: true, precision: 53 })
  logit: number | null;

  @ManyToOne(() => Dn, (dn) => dn.dnNns)
  @JoinColumn([{ name: "dn_id", referencedColumnName: "id" }])
  dn: Dn;

  @ManyToOne(() => Nn, (nn) => nn.dnNns, { onDelete: "CASCADE" })
  @JoinColumn([{ name: "nn_id", referencedColumnName: "id" }])
  nn: Nn;
}
