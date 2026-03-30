# TODO

- [x] Context tag in API
- [x] Context in invocability concepts

- Integrate geometric allocator (that can throw)
  - [x] Initial impl
  - [ ] Test correct throwing spec

- [ ] Optimize release/resume in presence of steals (need benchmark)

- [ ] `-fassume-nothrow-exception-dtor`

- [ ] Test nothrow allocator performance (just terminate?)

- [ ] Cancellation:
  - [ ] Maybe in separate `co_await scope`
  - [ ] Integrate into join
  - [ ] Exception safety
