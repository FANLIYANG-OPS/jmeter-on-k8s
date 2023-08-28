import typing
from typing import Optional, Type, cast

T = typing.TypeVar("T")


class ApiSpecError(Exception):
    pass


def typename(type: type) -> str:
    content_type_names = {"dict": "Map", "str": "String",
                          "int": "Integer", "bool": "Boolean", "list": "List"}
    if type.__name__ not in content_type_names:
        return type.__name__
    return content_type_names[type.__name__]


def do_get_str(d: dict, key: str, what: str, *, default_value: Optional[str] = None) -> str:
    return _do_get(d, key, what, default_value, str)


def do_get_dict(d: dict, key: str, what: str, default_value: Optional[dict] = None) -> dict:
    return _do_get(d, key, what, default_value, dict)


def _do_get(d: dict, key: str, what: str, default_value: Optional[T], expected_type: Type[T]) -> T:
    if default_value is None and key not in d:
        raise ApiSpecError(f"{what}.{key} is mandatory, but is not set")
    value = d.get(key, default_value)
    if not isinstance(value, expected_type):
        raise ApiSpecError(
            f"{what}.{key} expected to be a {typename(expected_type)} but is {typename(type(value)) if value is not None else 'not set'}")
    return cast(T, value)
