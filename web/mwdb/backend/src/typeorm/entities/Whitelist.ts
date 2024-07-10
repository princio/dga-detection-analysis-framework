import {
  Column,
  Entity,
  Index,
  OneToMany,
  PrimaryGeneratedColumn,
} from "typeorm";
import { WhitelistDn } from "./WhitelistDn";
import { WhitelistList } from "./WhitelistList";
import { WhitelistList_1 } from "./WhitelistList_1";

@Index("whitelist_pkey", ["id"], { unique: true })
@Index("whitelist_name_year_name1_year1_key", ["name", "year"], {
  unique: true,
})
@Entity("whitelist", { schema: "public" })
export class Whitelist {
  @PrimaryGeneratedColumn({ type: "integer", name: "id" })
  id: number;

  @Column("text", { name: "name", nullable: true, unique: true })
  name: string | null;

  @Column("integer", { name: "year", nullable: true, unique: true })
  year: number | null;

  @Column("integer", { name: "count", nullable: true })
  count: number | null;

  @OneToMany(() => WhitelistDn, (whitelistDn) => whitelistDn.whitelist)
  whitelistDns: WhitelistDn[];

  @OneToMany(() => WhitelistList, (whitelistList) => whitelistList.whitelist)
  whitelistLists: WhitelistList[];

  @OneToMany(
    () => WhitelistList_1,
    (whitelistList_1) => whitelistList_1.whitelist
  )
  whitelistListS: WhitelistList_1[];
}
