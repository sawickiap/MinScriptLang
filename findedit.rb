#!/usr/bin/ruby

require "ostruct"
require "optparse"
require "colorize"

PATTERNS = [
  "*.cpp",
  "*.c",
  "*.h",
  "include/*.h",
  "src/*.h",
  "src/*.c",
]

class Greppy
  Entry = Struct.new(:lineno, :rawline, :hline)

  def initialize(opts, files)
    @opts = opts
    @files = files
  end

  def maybe_suspicious(line)
    if line.match?(/[\"\'\`]/) || line.include?("##") || line.include?("#") then
      return true
    end
    return false
  end

  def grep(rx, file)
    success = false
    cmatch = 0
    suspicious = []
    File.open(file, "rb") do |fh|
      lno = 0
      fh.each_line do |line|
        line.rstrip!
        sc = line.scrub
        m = sc.match(rx)
        if m != nil then
          realno = (lno + 1)
          dline = line.dump
          # get rid of beginning and ending quote
          dline = dline[1 .. (dline.length - 1)]
          dline = dline[0 .. (dline.length - 2)]
          #tline = dline.gsub(/\\/, "")
          tline = dline
          hline = tline.gsub(rx){|itm| sprintf("%s".colorize(:red), itm) }
          if maybe_suspicious(line) then
            suspicious.push(Entry.new(realno, line, hline))
          end
          $stderr.printf("   -- %s:%d: ".colorize(:blue), file, lno+1);
          $stderr.printf("%s", hline)
          $stderr.printf("\n")
          success = true
          cmatch += 1
        end
        lno += 1
      end
    end
    if success then
      if suspicious.length > 0 then
        $stderr.printf("**WARNING** some lines look suspicious:\n")
        suspicious.each do |ent|
          $stderr.printf("  line %d: %s\n", ent.lineno, ent.hline)
        end
      end
      $stderr.printf("total: %d matches\n", cmatch)
      $stderr.printf("   ------------------\n")
    end
    return success
  end

  def rgrep(rx)
    rt = []
    @files.each do |f|
      if grep(rx, f) then
        if block_given? then
          yield f
        else
          rt.push(f)
        end
      end
    end
    return rt unless block_given?
  end

  def grepandedit(rx)
    rgrep(rx) do |f|
      if @opts.dryrun then
        $stderr.printf("\n")
      else
        if @opts.waitbeforerun then
          $stderr.printf("enter to start editing, or n(ext) to skip, or q(uit) to quit: ")
          r = $stdin.gets
          if (r != nil) then
            r.strip!
            if r.match?(/^q(uit)?/i) then
              $stderr.printf("ok; goodbye\n")
              exit(0)
            elsif r.match?(/^n(ext)?/i) then
              next
            end
          end
        end
        system("edit", f)
        $stderr.printf("[press enter to continue with the next file]")
        $stdin.gets
      end
    end
  end
end

begin
  regex = nil
  opts = OpenStruct.new({
    icase: false,
    dryrun: false,
    waitbeforerun: true,
  })
  OptionParser.new{|prs|
    prs.on("-i", "--ignorecase", "ignore case"){
      opts.icase = true
    }
    prs.on("-t", "-s", "--dry", "--test", "show results only, don't run anything"){
      opts.dryrun = true
    }
    prs.on("-c", "--noconfirm", "disable waiting for enter to start editing"){
      opts.waitbeforerun = false
    }
  }.parse!
  if ARGV.empty? then
    $stderr.printf("usage: %s [<opts>] <regex> [<another> ...]\n", File.basename($0))
  else
    files = []
    PATTERNS.each{|pat| files |= Dir.glob(pat) }
    g = Greppy.new(opts, files)
    ARGV.each do |rawrx|
      rx = if opts.icase then Regexp.new(rawrx, "i") else Regexp.new(rawrx) end
      g.grepandedit(rx)
    end
  end
end
