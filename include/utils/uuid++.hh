/*
**  MODIFIED BY <scott.e.graves@gmail.com>
**    - Modifications for C++11
**    - Memory leak avoidance
**
**  OSSP uuid - Universally Unique Identifier
**  Copyright (c) 2004-2008 Ralf S. Engelschall <rse@engelschall.com>
**  Copyright (c) 2004-2008 The OSSP Project <http://www.ossp.org/>
**
**  This file is part of OSSP uuid, a library for the generation
**  of UUIDs which can found at http://www.ossp.org/pkg/lib/uuid/
**
**  Permission to use, copy, modify, and distribute this software for
**  any purpose with or without fee is hereby granted, provided that
**  the above copyright notice and this permission notice appear in all
**  copies.
**
**  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
**  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
**  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
**  IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
**  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
**  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
**  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
**  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
**  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
**  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
**  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
**  SUCH DAMAGE.
**
**  uuid++.hh: library C++ API definition
*/
#if __linux__

#ifndef __UUIDXX_HH__
#define __UUIDXX_HH__
#include <cstdarg>
#include <string>
#include <exception>
#include <vector>
#include <uuid.h>

namespace repertory {
/* UUID exception class */
class uuid_error_t : public virtual std::exception {
public:
  uuid_error_t() : rc(UUID_RC_OK){};

  explicit uuid_error_t(const uuid_rc_t &code) : rc(code) {}

private:
  const uuid_rc_t rc;

public:
  uuid_rc_t code() const { return rc; };

  const char *what() const noexcept override {
    static std::string ret;
    if (ret.empty()) {
      auto *p = uuid_error(rc);
      ret = std::string(p, strlen(p));
      free(p);
    }
    return ret.c_str();
  }
};

/* UUID object class */
class uuid {
public:
  /* construction & destruction */

  /* standard constructor */
  uuid() {
    uuid_rc_t rc;
    if ((rc = uuid_create(&ctx)) != UUID_RC_OK)
      throw uuid_error_t(rc);
  }

  /* copy     constructor */
  uuid(const uuid &obj) {
    /* Notice: the copy constructor is the same as the assignment
   operator (with the object as the argument) below, except that
   (1) no check for self-assignment is required, (2) no existing
   internals have to be destroyed and (3) no return value is given back. */
    uuid_rc_t rc;
    if ((rc = uuid_clone(obj.ctx, &ctx)) != UUID_RC_OK)
      throw uuid_error_t(rc);
  }

  /* import   constructor */
  explicit uuid(const uuid_t *obj) {
    uuid_rc_t rc;
    if (obj == nullptr)
      throw uuid_error_t(UUID_RC_ARG);
    if ((rc = uuid_clone(obj, &ctx)) != UUID_RC_OK)
      throw uuid_error_t(rc);
  }

  /* import   constructor */
  explicit uuid(const std::vector<void *> &bin) {
    uuid_rc_t rc;
    if (bin.empty())
      throw uuid_error_t(UUID_RC_ARG);
    if ((rc = uuid_create(&ctx)) != UUID_RC_OK)
      throw uuid_error_t(rc);
    import(bin);
  }

  /* import   constructor */
  explicit uuid(const std::string &str) {
    uuid_rc_t rc;
    if (str.empty())
      throw uuid_error_t(UUID_RC_ARG);
    if ((rc = uuid_create(&ctx)) != UUID_RC_OK)
      throw uuid_error_t(rc);
    import(str);
  }

  /* move     constructor */
  uuid(uuid &&obj) noexcept : ctx(obj.ctx) {}

  /* destructor */
  ~uuid() { uuid_destroy(ctx); }

private:
  uuid_t *ctx = nullptr;

public:
  /* copying & cloning */

  /* copy assignment operator */
  uuid &operator=(const uuid &obj) {
    uuid_rc_t rc;
    if (this != &obj) {
      if ((rc = uuid_destroy(ctx)) != UUID_RC_OK)
        throw uuid_error_t(rc);
      if ((rc = uuid_clone(obj.ctx, &ctx)) != UUID_RC_OK)
        throw uuid_error_t(rc);
    }
    return *this;
  }

  /* move assignment operator */
  uuid &operator=(uuid &&obj) {
    if (this != &obj) {
      ctx = obj.ctx;
    }
    return *this;
  }

  /* import assignment operator */
  uuid &operator=(const uuid_t *obj) {
    uuid_rc_t rc;
    if (obj == nullptr)
      throw uuid_error_t(UUID_RC_ARG);
    if ((rc = uuid_clone(obj, &ctx)) != UUID_RC_OK)
      throw uuid_error_t(rc);
    return *this;
  }

  /* import assignment operator */
  uuid &operator=(const std::vector<void *> &bin) {
    if (bin.empty())
      throw uuid_error_t(UUID_RC_ARG);
    import(bin);
    return *this;
  }

  /* import assignment operator */
  uuid &operator=(const std::string &str) {
    if (str.empty())
      throw uuid_error_t(UUID_RC_ARG);
    import(str);
    return *this;
  }

  /* regular method */
  uuid clone() { return uuid(*this); }

  /* content generation */

  /* regular method */
  void load(const std::string &name) {
    uuid_rc_t rc;
    if (name.empty())
      throw uuid_error_t(UUID_RC_ARG);
    if ((rc = uuid_load(ctx, &name[0])) != UUID_RC_OK)
      throw uuid_error_t(rc);
  }

  /* regular method */
  void make(unsigned int mode, ...) {
    uuid_rc_t rc;
    va_list ap;

    va_start(ap, mode);
    if ((mode & UUID_MAKE_V3) || (mode & UUID_MAKE_V5)) {
      const uuid *ns = (const uuid *)va_arg(ap, const uuid *);
      const char *name = (const char *)va_arg(ap, char *);
      if (ns == nullptr || name == nullptr)
        throw uuid_error_t(UUID_RC_ARG);
      rc = uuid_make(ctx, mode, ns->ctx, name);
    } else
      rc = uuid_make(ctx, mode);
    va_end(ap);
    if (rc != UUID_RC_OK)
      throw uuid_error_t(rc);
  }

  /* content comparison */

  /* regular method */
  int isnil() {
    uuid_rc_t rc;
    int rv;

    if ((rc = uuid_isnil(ctx, &rv)) != UUID_RC_OK)
      throw uuid_error_t(rc);
    return rv;
  }

  /* regular method */
  int compare(const uuid &obj) {
    uuid_rc_t rc;
    int rv;

    if ((rc = uuid_compare(ctx, obj.ctx, &rv)) != UUID_RC_OK)
      throw uuid_error_t(rc);
    return rv;
  }

  /* comparison operator */
  int operator==(const uuid &obj) { return (compare(obj) == 0); }

  /* comparison operator */
  int operator!=(const uuid &obj) { return (compare(obj) != 0); }

  /* comparison operator */
  int operator<(const uuid &obj) { return (compare(obj) < 0); }

  /* comparison operator */
  int operator<=(const uuid &obj) { return (compare(obj) <= 0); }

  /* comparison operator */
  int operator>(const uuid &obj) { return (compare(obj) > 0); }

  /* comparison operator */
  int operator>=(const uuid &obj) { return (compare(obj) >= 0); }

  /* content importing & exporting */

  /* regular method */
  void import(const std::vector<void *> &bin) {
    uuid_rc_t rc;
    if ((rc = uuid_import(ctx, UUID_FMT_BIN, &bin[0], UUID_LEN_BIN)) != UUID_RC_OK)
      throw uuid_error_t(rc);
  }

  /* regular method */
  void import(const std::string &str) {
    uuid_rc_t rc;
    if ((rc = uuid_import(ctx, UUID_FMT_STR, &str[0], UUID_LEN_STR)) != UUID_RC_OK)
      if ((rc = uuid_import(ctx, UUID_FMT_SIV, &str[0], UUID_LEN_SIV)) != UUID_RC_OK)
        throw uuid_error_t(rc);
  }

  /* regular method */
  std::vector<void *> binary() {
    uuid_rc_t rc;
    void *bin = nullptr;
    if ((rc = uuid_export(ctx, UUID_FMT_BIN, &bin, nullptr)) != UUID_RC_OK)
      throw uuid_error_t(rc);

    std::vector<void *> data;
    data.resize(UUID_LEN_BIN);
    memcpy(&data[0], bin, UUID_LEN_BIN);
    free(bin);
    return data;
  }

  /* regular method */
  std::string string() {
    uuid_rc_t rc;
    char *str = nullptr;
    if ((rc = uuid_export(ctx, UUID_FMT_STR, (void **)&str, nullptr)) != UUID_RC_OK)
      throw uuid_error_t(rc);
    std::string data;
    data.resize(UUID_LEN_STR + 1);
    memcpy(&data[0], str, UUID_LEN_STR);
    free(str);
    return &data[0];
  }

  /* regular method */
  std::string integer() {
    uuid_rc_t rc;
    char *str = nullptr;
    if ((rc = uuid_export(ctx, UUID_FMT_SIV, (void **)&str, nullptr)) != UUID_RC_OK)
      throw uuid_error_t(rc);
    std::string data;
    data.resize(UUID_LEN_SIV + 1);
    memcpy(&data[0], str, UUID_LEN_SIV);
    free(str);
    return &data[0];
  }

  /* regular method */
  std::string summary() {
    uuid_rc_t rc;
    char *txt = nullptr;
    if ((rc = uuid_export(ctx, UUID_FMT_TXT, (void **)&txt, nullptr)) != UUID_RC_OK)
      throw uuid_error_t(rc);
    std::string data(txt, strlen(txt));
    free(txt);
    return data;
  }

  /* regular method */
  unsigned long version() { return uuid_version(); }
};
} // namespace repertory
#endif /* __UUIDXX_HH__ */

#endif
