import {
  Column,
  Entity,
  Index,
  JoinColumn,
  ManyToOne,
  PrimaryGeneratedColumn,
} from "typeorm";
import { Dn } from "./Dn";
import { Whitelist } from "./Whitelist";

@Index("whitelist_dn_dn_id_whitelist_id_key1", ["dnId", "whitelistId"], {
  unique: true,
})
@Index("whitelist_dn_dn_id_whitelist_id_key", ["dnId", "whitelistId"], {
  unique: true,
})
@Index("pk_whitelist_dn", ["id"], { unique: true })
@Entity("whitelist_dn", { schema: "public" })
export class WhitelistDn {
  @PrimaryGeneratedColumn({ type: "bigint", name: "id" })
  id: string;

  @Column("bigint", { name: "dn_id", nullable: true })
  dnId: string | null;

  @Column("bigint", { name: "whitelist_id" })
  whitelistId: string;

  @Column("integer", { name: "rank_dn", nullable: true })
  rankDn: number | null;

  @Column("integer", { name: "rank_bdn", nullable: true })
  rankBdn: number | null;

  @ManyToOne(() => Dn, (dn) => dn.whitelistDns)
  @JoinColumn([{ name: "dn_id", referencedColumnName: "id" }])
  dn: Dn;

  @ManyToOne(() => Whitelist, (whitelist) => whitelist.whitelistDns)
  @JoinColumn([{ name: "whitelist_id", referencedColumnName: "id" }])
  whitelist: Whitelist;
}
